// Copyright 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "media/webrtc/neva/webrtc_pass_through_video_decoder.h"

#include <mutex>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/aligned_memory.h"
#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "media/base/neva/media_platform_prefs.h"
#include "media/base/video_frame.h"
#include "third_party/blink/renderer/platform/webrtc/webrtc_video_frame_adapter.h"
#include "third_party/webrtc/api/video_codecs/sdp_video_format.h"
#include "third_party/webrtc/modules/video_coding/include/video_error_codes.h"
#include "third_party/webrtc/rtc_base/helpers.h"
#include "third_party/webrtc/rtc_base/ref_counted_object.h"

namespace media {

namespace {

const char* kImplementationName = "WebRtcPassThroughVideoDecoder";

// Maximum number of frames that we will queue in |pending_frames_|.
const int32_t kMaxPendingBuffers = 8;

// Maximum number of timestamps that will be maintained in |decode_timestamps_|.
// Really only needs to be a bit larger than the maximum reorder distance (which
// is presumably 0 for WebRTC), but being larger doesn't hurt much.
const int32_t kMaxDecodeHistory = 32;

// Maximum number of consecutive frames that can fail to decode before
// requesting fallback to software decode.
const int32_t kMaxConsecutiveErrors = 5;

// Map webrtc::VideoCodecType to media::VideoCodec.
media::VideoCodec ToVideoCodec(webrtc::VideoCodecType webrtc_codec) {
  switch (webrtc_codec) {
    case webrtc::kVideoCodecVP8:
      return media::kCodecVP8;
    case webrtc::kVideoCodecVP9:
      return media::kCodecVP9;
    case webrtc::kVideoCodecH264:
      return media::kCodecH264;
    default:
      break;
  }
  return media::kUnknownVideoCodec;
}

// Map webrtc::VideoCodecType to media::VideoPixelFormat.
media::VideoPixelFormat ToVideoPixelFormat(
    const webrtc::VideoCodecType video_codec_type) {
  switch (video_codec_type) {
    case webrtc::kVideoCodecVP8:
      return media::PIXEL_FORMAT_VP8;
    case webrtc::kVideoCodecVP9:
      return media::PIXEL_FORMAT_VP9;
    case webrtc::kVideoCodecH264:
      return media::PIXEL_FORMAT_H264;
    default:
      break;
  }
  return media::PIXEL_FORMAT_UNKNOWN;
}

}

typedef std::map<uint32_t, WebRtcPassThroughVideoDecoder*> WebRtcDecoderMap;
WebRtcDecoderMap webrtc_video_decoder_map_;

bool WebRtcPassThroughVideoDecoder::media_decoder_available_ = true;

// static
std::unique_ptr<WebRtcPassThroughVideoDecoder>
WebRtcPassThroughVideoDecoder::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    const webrtc::SdpVideoFormat& sdp_format) {
  DVLOG(1) << __func__ << "(" << sdp_format.name << ")";

  const webrtc::VideoCodecType webrtc_codec_type =
      webrtc::PayloadStringToCodecType(sdp_format.name);

  // Bail early for unknown codecs.
  media::VideoCodec video_codec = ToVideoCodec(webrtc_codec_type);
  if (video_codec == media::kUnknownVideoCodec)
    return nullptr;

  // Fallback to software decoder if not supported by platform.
  if (!MediaPlatformPrefs::Get()->IsCodecSupported(video_codec))
    return nullptr;

  VideoPixelFormat video_pixel_format = ToVideoPixelFormat(webrtc_codec_type);
  return base::WrapUnique(new WebRtcPassThroughVideoDecoder(
      main_task_runner, video_codec, video_pixel_format));
}

// static
void WebRtcPassThroughVideoDecoder::SetMediaDecoderAvailable(bool available) {
  LOG(INFO) << __func__ << " available: " << (available ? " TRUE" : " FALSE");
  media_decoder_available_ = available;
}

WebRtcPassThroughVideoDecoder::WebRtcPassThroughVideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    media::VideoCodec video_codec,
    media::VideoPixelFormat video_pixel_format)
    : main_task_runner_(main_task_runner),
      video_codec_(video_codec),
      video_pixel_format_(video_pixel_format) {
  LOG(INFO) << __func__ << " [" << this << "] "
                        << " video_pixel_format: " << video_pixel_format
                        << " codec: " << GetCodecName(video_codec);
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

WebRtcPassThroughVideoDecoder::~WebRtcPassThroughVideoDecoder() {
  LOG(INFO) << __func__ <<  "[" << this << "] ";
}

int32_t WebRtcPassThroughVideoDecoder::InitDecode(
    const webrtc::VideoCodec* codec_settings,
    int32_t number_of_cores) {
  VLOG(1) << __func__ << " codec: " << GetCodecName(video_codec_);

  if (codec_settings == nullptr)
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;

  video_codec_type_ = codec_settings->codecType;

  // Always start with a complete key frame.
  key_frame_required_ = true;
  return media_decoder_available_ ? WEBRTC_VIDEO_CODEC_OK
                                  : WEBRTC_VIDEO_CODEC_UNINITIALIZED;
}

int32_t WebRtcPassThroughVideoDecoder::Decode(
    const webrtc::EncodedImage& input_image,
    bool missing_frames,
    int64_t render_time_ms) {
  // Check s/w fallback for only once, the first time this is called.
  if (!media_decoder_acquired_) {
    if (!media_decoder_available_) {
      // Fallback to software mode if no free media decoder.
      LOG(INFO) << __func__ << " Fallback to s/w Decoder";
      return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
    }
    media_decoder_acquired_ = true;
  }

  // Hardware VP9 decoders don't handle more than one spatial layer. Fall back
  // to software decoding. See https://crbug.com/webrtc/9304.
  if (video_codec_type_ == webrtc::kVideoCodecVP9 &&
      input_image.SpatialIndex().value_or(0) > 0) {
    LOG(INFO) << __func__
              << " VP9 with more spatial index > 0. Fallback to s/w Decoder";
    return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
  }

  if (missing_frames || !input_image._completeFrame) {
    VLOG(1) << "Missing or incomplete frames";
    // We probably can't handle broken frames. Request a key frame.
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (key_frame_required_) {
    // We discarded previous frame because we have too many pending framess
    // (see logic) below. Now we need to wait for the key frame and discard
    // everything else.
    if (input_image._frameType != webrtc::VideoFrameType::kVideoFrameKey) {
      DVLOG(2) << "Discard non-key frame";
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    DVLOG(2) << "Key frame received, resume decoding";
    // ok, we got key frame and can continue decoding
    key_frame_required_ = false;
  }

  bool key_frame = input_image._frameType
                   == webrtc::VideoFrameType::kVideoFrameKey;
  if (key_frame) {
    frame_size_.set_width(input_image._encodedWidth);
    frame_size_.set_height(input_image._encodedHeight);
    LOG(INFO) << __func__ << " key_frame_size: " << frame_size_.ToString();
  }

  std::unique_ptr<uint8_t, base::AlignedFreeDeleter> encoded_data(
      static_cast<uint8_t*>(
          base::AlignedAlloc(input_image.size(),
              media::VideoFrameLayout::kBufferAddressAlignment)));
  memcpy(encoded_data.get(), input_image.data(), input_image.size());

  base::TimeDelta timestamp_ms =
      base::TimeDelta::FromMicroseconds(input_image.Timestamp());
  // Make a shallow copy.
  scoped_refptr<media::VideoFrame> encoded_frame =
      media::VideoFrame::WrapExternalData(video_pixel_format_,
                                          frame_size_,
                                          gfx::Rect(frame_size_),
                                          frame_size_,
                                          encoded_data.get(),
                                          input_image.size(),
                                          timestamp_ms);
  if (!encoded_frame) {
    LOG(ERROR) << __func__ << " Could not allocate encoded_frame.";
    return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
  }

  // The bind ensures that we keep a pointer to the encoded data.
  encoded_frame->AddDestructionObserver(
      base::Bind(&base::AlignedFree, encoded_data.release()));
  encoded_frame->metadata()->SetBoolean(media::VideoFrameMetadata::KEY_FRAME,
                                        key_frame);

  // Queue for decoding.
  {
    base::AutoLock auto_lock(lock_);

    if (pending_frames_.size() >= kMaxPendingBuffers) {
      // We are severely behind. Drop pending frames and request a keyframe to
      // catch up as quickly as possible.
      DVLOG(2) << "Pending VideoFrames overflow";
      pending_frames_.clear();

      // Actually we just discarded a frame. We must wait for the key frame and
      // drop any other non-key frame.
      key_frame_required_ = true;
      if (++consecutive_error_count_ > kMaxConsecutiveErrors) {
        decode_timestamps_.clear();
        return WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
      }
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    pending_frames_.push_back(std::move(encoded_frame));
  }

  main_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcPassThroughVideoDecoder::DecodeOnMediaThread,
                     weak_this_));

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebRtcPassThroughVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  VLOG(1) << __func__ << " decoder available: " << media_decoder_available_;
  return media_decoder_available_ ? WEBRTC_VIDEO_CODEC_OK
                                  : WEBRTC_VIDEO_CODEC_UNINITIALIZED;
}

int32_t WebRtcPassThroughVideoDecoder::Release() {
  VLOG(1) << __func__ << " decoder available: " << media_decoder_available_;

  base::AutoLock auto_lock(lock_);
  pending_frames_.clear();
  decode_timestamps_.clear();

  media_decoder_acquired_ = false;
  return media_decoder_available_ ? WEBRTC_VIDEO_CODEC_OK
                                  : WEBRTC_VIDEO_CODEC_UNINITIALIZED;
}

const char* WebRtcPassThroughVideoDecoder::ImplementationName() const {
  return kImplementationName;
}

void WebRtcPassThroughVideoDecoder::DecodeOnMediaThread() {
  std::deque<scoped_refptr<media::VideoFrame>> pending_frames;
  {
    base::AutoLock auto_lock(lock_);
    pending_frames.swap(pending_frames_);
  }

  while (!pending_frames.empty()) {
    scoped_refptr<media::VideoFrame> pending_frame = pending_frames.front();

    // Record the timestamp.
    while (decode_timestamps_.size() >= kMaxDecodeHistory)
      decode_timestamps_.pop_front();
    decode_timestamps_.push_back(pending_frame->timestamp());

    ReturnEncodedFrame(pending_frame);
    pending_frames.pop_front();
  }
}

void WebRtcPassThroughVideoDecoder::ReturnEncodedFrame(
    scoped_refptr<media::VideoFrame> encoded_frame) {
  const base::TimeDelta timestamp = encoded_frame->timestamp();
  webrtc::VideoFrame rtc_frame =
      webrtc::VideoFrame::Builder()
          .set_video_frame_buffer(
              new rtc::RefCountedObject<blink::WebRtcVideoFrameAdapter>(
                  std::move(encoded_frame)))
          .set_timestamp_rtp(static_cast<uint32_t>(timestamp.InMicroseconds()))
          .set_timestamp_us(0)
          .set_rotation(webrtc::kVideoRotation_0)
          .build();

  if (!base::Contains(decode_timestamps_, timestamp)) {
    LOG(INFO) << __func__ << " Discarding frame with timestamp: " << timestamp;
    return;
  }

  decode_complete_callback_->Decoded(rtc_frame);
  consecutive_error_count_ = 0;
}

}  // namespace media