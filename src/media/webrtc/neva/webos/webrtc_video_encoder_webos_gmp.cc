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

#include "media/webrtc/neva/webos/webrtc_video_encoder_webos_gmp.h"

#pragma GCC optimize("rtti")
#include <cmp/media_encoder_client.h>
#pragma GCC reset_options

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/video_bitrate_allocation.h"
#include "media/base/video_frame.h"
#include "media/video/h264_parser.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/webrtc/webrtc_video_frame_adapter.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"
#include "third_party/webrtc/rtc_base/time_utils.h"

namespace media {

namespace {

CMP_VIDEO_CODEC emc_codecs[] = {
  CMP_VIDEO_CODEC_NONE,
  CMP_VIDEO_CODEC_VP8,
  CMP_VIDEO_CODEC_VP9,
  CMP_VIDEO_CODEC_H264,
  CMP_VIDEO_CODEC_MAX,
};

const char* EncoderCbTypeToString(gint type) {
#define STRINGIFY_ENCODER_CB_TYPE_CASE(type) \
  case type:                             \
    return #type

  switch (static_cast<ENCODER_CB_TYPE_T>(type)) {
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_LOAD_COMPLETE);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_PLAYING);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_PAUSED);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_BUFFER_ENCODED);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_EOS);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_NOTIFY_ERROR);
    STRINGIFY_ENCODER_CB_TYPE_CASE(ENCODER_CB_UNLOAD_COMPLETE);
    default:
      return "Unknown CB type";
  }
}

// Populates struct webrtc::RTPFragmentationHeader for H264 codec.
// Each entry specifies the offset and length (excluding start code) of a NALU.
// Returns true if successful.
bool GetRTPFragmentationHeaderH264(webrtc::RTPFragmentationHeader* header,
                                   const uint8_t* data,
                                   uint32_t length) {
  std::vector<media::H264NALU> nalu_vector;
  if (!media::H264Parser::ParseNALUs(data, length, &nalu_vector)) {
    LOG(ERROR) << __func__ << " H264Parser::ParseNALUs() failed.";
    return false;
  }

  header->VerifyAndAllocateFragmentationHeader(nalu_vector.size());
  for (size_t i = 0; i < nalu_vector.size(); ++i) {
    header->fragmentationOffset[i] = nalu_vector[i].data - data;
    header->fragmentationLength[i] = static_cast<size_t>(nalu_vector[i].size);
  }
  return true;
}

}

// static
WebRtcVideoEncoder* WebRtcVideoEncoder::Create(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type) {
  return new WebRtcVideoEncoderWebOSGMP(
      main_task_runner, encode_task_runner, video_codec_type,
      video_content_type);
}

// static
bool WebRtcVideoEncoder::IsCodecTypeSupported(
    webrtc::VideoCodecType type) {
  return cmp::player::MediaEncoderClient::IsCodecSupported(emc_codecs[type]);
}

// static
const char* WebRtcVideoEncoder::ImplementationName() {
  return "WebRtcVideoEncoderWebOSGMP";
}

// static
void WebRtcVideoEncoderWebOSGMP::Callback(const gint cb_type,
                                          const void* cb_data,
                                          void* user_data) {
  WebRtcVideoEncoderWebOSGMP* that =
      static_cast<WebRtcVideoEncoderWebOSGMP*>(user_data);
  if (!that) {
    LOG(ERROR) << __func__ << " Invalid user pointer recieved.";
    return;
  }

  blink::PostCrossThreadTask(
      *that->main_task_runner_.get(), FROM_HERE,
      WTF::CrossThreadBindOnce(
          &WebRtcVideoEncoderWebOSGMP::DispatchCallback,
          scoped_refptr<WebRtcVideoEncoderWebOSGMP>(that),
          cb_type, WTF::CrossThreadUnretained(cb_data)));
}

WebRtcVideoEncoderWebOSGMP::WebRtcVideoEncoderWebOSGMP(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> encode_task_runner,
    webrtc::VideoCodecType video_codec_type,
    webrtc::VideoContentType video_content_type)
    : WebRtcVideoEncoder(encode_task_runner, video_codec_type,
                         video_content_type),
      main_task_runner_(main_task_runner) {
  LOG(INFO) << __func__ << " codecType=" << video_codec_type;
}

WebRtcVideoEncoderWebOSGMP::~WebRtcVideoEncoderWebOSGMP() {
  DCHECK(!media_encoder_client_);
}

void WebRtcVideoEncoderWebOSGMP::CreateAndInitialize(
    const gfx::Size& input_visible_size,
    uint32_t bitrate,
    uint32_t framerate,
    base::WaitableEvent* async_waiter,
    int32_t* async_retval) {
  LOG(INFO) << __func__
            << ", input_size=" << input_visible_size.ToString()
            << ", framerate=" << framerate << ", bitrate=" << bitrate;

  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  SetStatus(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
  RegisterAsyncWaiter(async_waiter, async_retval);

  // Check for overflow converting bitrate (kilobits/sec) to bits/sec.
  if (IsBitrateTooHigh(bitrate)) {
    LOG(ERROR) << __func__ << " Bitrate is too high";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  input_visible_size_ = input_visible_size;
  i420_buffer_size_ = webrtc::CalcBufferSize(webrtc::VideoType::kI420,
                                             input_visible_size_.width(),
                                             input_visible_size_.height());
  i420_buffer_.reset(new uint8_t[i420_buffer_size_]);
  if (!i420_buffer_) {
    LOG(ERROR) << __func__ << " Failed to allocate buffer";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  media_encoder_client_.reset(new cmp::player::MediaEncoderClient());
  if (!media_encoder_client_) {
    LOG(ERROR) << __func__ << " Failed to create encoder instance";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  media_encoder_client_->RegisterCallback(&WebRtcVideoEncoderWebOSGMP::Callback,
                                          this);

  ENCODER_INIT_DATA_T init_data;
  init_data.pixelFormat = CMP_PIXEL_I420;
  init_data.codecFormat = emc_codecs[video_codec_type_];
  init_data.width = input_visible_size_.width();
  init_data.height = input_visible_size_.height();
  init_data.frameRate = framerate;

  if (!media_encoder_client_->Init(&init_data)) {
    LOG(ERROR) << __func__ << " Error initializing encoder.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
    return;
  }

  main_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcVideoEncoderWebOSGMP::NotifyLoadCompleted,
                     base::Unretained(this)));
}

void WebRtcVideoEncoderWebOSGMP::EncodeFrame(
    const webrtc::VideoFrame* new_frame,
    bool force_keyframe,
    base::WaitableEvent* async_waiter,
    int32_t* async_retval) {
  DVLOG(3) << __func__;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  RegisterAsyncWaiter(async_waiter, async_retval);
  int32_t retval = GetStatus();
  if (retval != WEBRTC_VIDEO_CODEC_OK) {
    SignalAsyncWaiter(retval);
    return;
  }

  if (!media_encoder_client_) {
    LOG(ERROR) << __func__ << " Error encoder client not created.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  const webrtc::VideoFrame* next_frame = new_frame;
  if (!i420_buffer_.get()) {
    LOG(ERROR) << __func__ << " Error allocation i420_buffer.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  if (i420_buffer_size_ != webrtc::ExtractBuffer(*next_frame,
                                                 i420_buffer_size_,
                                                 i420_buffer_.get())) {
    LOG(ERROR) << __func__ << " Error extracting i420_buffer.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  if (CMP_MEDIA_OK != media_encoder_client_->Encode(i420_buffer_.get(),
                                                    i420_buffer_size_)) {
    LOG(ERROR) << __func__ << " Error feeding i420_buffer.";
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_ERROR);
    return;
  }

  SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
}

void WebRtcVideoEncoderWebOSGMP::RequestEncodingParametersChange(
    const webrtc::VideoEncoder::RateControlParameters& parameters) {
  DVLOG(3) << __func__ << " bitrate=" << parameters.bitrate.ToString()
           << ", framerate=" << parameters.framerate_fps;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  // This is a workaround to zero being temporarily provided, as part of the
  // initial setup, by WebRTC.
  if (media_encoder_client_) {
    media::VideoBitrateAllocation allocation;
    if (parameters.bitrate.get_sum_bps() == 0) {
      allocation.SetBitrate(0, 0, 1);
    }
    uint32_t framerate =
        std::max(1u, static_cast<uint32_t>(parameters.framerate_fps + 0.5));
    for (size_t spatial_id = 0;
         spatial_id < media::VideoBitrateAllocation::kMaxSpatialLayers;
         ++spatial_id) {
      for (size_t temporal_id = 0;
           temporal_id < media::VideoBitrateAllocation::kMaxTemporalLayers;
           ++temporal_id) {
        // TODO(sprang): Clean this up if/when webrtc struct moves to int.
        uint32_t layer_bitrate =
            parameters.bitrate.GetBitrate(spatial_id, temporal_id);
        CHECK_LE(layer_bitrate,
                 static_cast<uint32_t>(std::numeric_limits<int>::max()));
        if (!allocation.SetBitrate(spatial_id, temporal_id, layer_bitrate)) {
          LOG(WARNING) << "Overflow in bitrate allocation: "
                       << parameters.bitrate.ToString();
          break;
        }
      }
    }

    ENCODING_PARAMS_T params;
    params.bitRate = allocation.GetSumBps();
    params.frameRate = framerate;
    media_encoder_client_->UpdateEncodingParams(&params);
  }
}

void WebRtcVideoEncoderWebOSGMP::Destroy(base::WaitableEvent* async_waiter) {
  DVLOG(3) << __func__;
  DCHECK(encode_task_runner_->BelongsToCurrentThread());

  if (media_encoder_client_) {
    media_encoder_client_.reset();
    SetStatus(WEBRTC_VIDEO_CODEC_UNINITIALIZED);
  }

  async_waiter->Signal();
}

bool WebRtcVideoEncoderWebOSGMP::IsBitrateTooHigh(uint32_t bitrate) {
  if (base::IsValueInRangeForNumericType<uint32_t>(bitrate * UINT64_C(1000)))
    return false;
  return true;
}

void WebRtcVideoEncoderWebOSGMP::ReturnEncodedImage(
    const webrtc::EncodedImage& image) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(3) << __func__;

  if (!encoded_image_callback_) {
    LOG(ERROR) << __func__ << " webrtc::EncodedImageCallback is not set";
    return;
  }

  webrtc::RTPFragmentationHeader header;
  switch (video_codec_type_) {
    case webrtc::kVideoCodecVP8:
    case webrtc::kVideoCodecVP9:
      // Generate a header describing a single fragment.
      header.VerifyAndAllocateFragmentationHeader(1);
      header.fragmentationOffset[0] = 0;
      header.fragmentationLength[0] = image.size();
      break;
    case webrtc::kVideoCodecH264:
      if (!GetRTPFragmentationHeaderH264(&header, image.data(), image.size())) {
        LOG(ERROR) << "Failed to get RTP fragmentation header for H264";
        return;
      }
      break;
    default:
      LOG(ERROR) << __func__ << "Invalid video codec type";
      return;
  }

  webrtc::CodecSpecificInfo info;
  info.codecType = video_codec_type_;
  if (video_codec_type_ == webrtc::kVideoCodecVP8) {
    info.codecSpecific.VP8.keyIdx = -1;
  } else if (video_codec_type_ == webrtc::kVideoCodecVP9) {
    bool key_frame = image._frameType == webrtc::VideoFrameType::kVideoFrameKey;
    info.codecSpecific.VP9.inter_pic_predicted = !key_frame;
    info.codecSpecific.VP9.flexible_mode = false;
    info.codecSpecific.VP9.ss_data_available = key_frame;
    info.codecSpecific.VP9.temporal_idx = webrtc::kNoTemporalIdx;
    info.codecSpecific.VP9.temporal_up_switch = true;
    info.codecSpecific.VP9.inter_layer_predicted = false;
    info.codecSpecific.VP9.gof_idx = 0;
    info.codecSpecific.VP9.num_spatial_layers = 1;
    info.codecSpecific.VP9.first_frame_in_picture = true;
    info.codecSpecific.VP9.end_of_picture = true;
    info.codecSpecific.VP9.spatial_layer_resolution_present = false;
    if (info.codecSpecific.VP9.ss_data_available) {
      info.codecSpecific.VP9.spatial_layer_resolution_present = true;
      info.codecSpecific.VP9.width[0] = image._encodedWidth;
      info.codecSpecific.VP9.height[0] = image._encodedHeight;
      info.codecSpecific.VP9.gof.num_frames_in_gof = 1;
      info.codecSpecific.VP9.gof.temporal_idx[0] = 0;
      info.codecSpecific.VP9.gof.temporal_up_switch[0] = false;
      info.codecSpecific.VP9.gof.num_ref_pics[0] = 1;
      info.codecSpecific.VP9.gof.pid_diff[0][0] = 1;
    }
  }

  const auto result =
      encoded_image_callback_->OnEncodedImage(image, &info, &header);
  if (result.error != webrtc::EncodedImageCallback::Result::OK) {
    LOG(ERROR) << __func__ << " : webrtc::EncodedImageCallback::Result.error = "
               << result.error;
  }
}

void WebRtcVideoEncoderWebOSGMP::NotifyLoadCompleted() {
  DVLOG(3) << __func__;
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (!load_completed_) {
    SetStatus(WEBRTC_VIDEO_CODEC_OK);
    SignalAsyncWaiter(WEBRTC_VIDEO_CODEC_OK);
    load_completed_ = true;
  }
}

void WebRtcVideoEncoderWebOSGMP::NotifyEncodedBufferReady(const void* data) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (!data) {
    LOG(ERROR) << __func__ << " Invalid buffer pointer";
    return;
  }

  const ENCODED_BUFFER_T* encoded_buffer =
          static_cast<const ENCODED_BUFFER_T*>(data);

  DVLOG(3) << __func__ << ", buffer_size=" << encoded_buffer->bufferSize
           << ", is_key_frame=" << encoded_buffer->isKeyFrame;

  const int64_t current_time_ms =
      rtc::TimeMicros() / base::Time::kMicrosecondsPerMillisecond;
  // RTP timestamp can wrap around. Get the lower 32 bits.
  uint32_t rtp_timestamp = static_cast<uint32_t>(current_time_ms * 90);

  webrtc::EncodedImage image;
  image.SetEncodedData(webrtc::EncodedImageBuffer::Create(
      static_cast<const uint8_t*>(encoded_buffer->encodedBuffer),
      encoded_buffer->bufferSize));
  image._encodedWidth = input_visible_size_.width();
  image._encodedHeight = input_visible_size_.height();
  image.SetTimestamp(rtp_timestamp);
  image.capture_time_ms_ = current_time_ms;
  image._frameType =
      (encoded_buffer->isKeyFrame ? webrtc::VideoFrameType::kVideoFrameKey
                                  : webrtc::VideoFrameType::kVideoFrameDelta);
  image.content_type_ = video_content_type_;
  image._completeFrame = true;

  ReturnEncodedImage(image);
}

void WebRtcVideoEncoderWebOSGMP::NotifyEncoderError(const void* data) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (!data) {
    LOG(ERROR) << __func__ << " Invalid buffer pointer";
    return;
  }

  const ENCODING_ERRORS_T* error_data =
      static_cast<const ENCODING_ERRORS_T*>(data);

  int32_t retval = WEBRTC_VIDEO_CODEC_ERROR;
  switch (error_data->errorCode) {
    case MEDIA_MSG_ERR_LOAD:
    case MEDIA_MSG_ERR_POLICY:
      retval = WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
      break;
    case MEDIA_MSG_ERR_PLAYING:
      retval = WEBRTC_VIDEO_CODEC_ERROR;
      break;
    default:
      NOTREACHED() << " errorCode: " << error_data->errorCode << " Not handled";
      break;
  }

  media_encoder_client_.reset();

  SetStatus(retval);
  if (async_waiter_)
    SignalAsyncWaiter(retval);
}

void WebRtcVideoEncoderWebOSGMP::DispatchCallback(const gint cb_type,
                                                  const void* cb_data) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  VLOG(1) << __func__ << " : " << EncoderCbTypeToString(cb_type);
  switch (static_cast<ENCODER_CB_TYPE_T>(cb_type)) {
    case ENCODER_CB_LOAD_COMPLETE: {
      NotifyLoadCompleted();
      break;
    }

    case ENCODER_CB_BUFFER_ENCODED: {
      NotifyEncodedBufferReady(cb_data);
      break;
    }

    case ENCODER_CB_NOTIFY_ERROR: {
      NotifyEncoderError(cb_data);
      break;
    }
    default:
      NOTREACHED() << " cb_type: " << cb_type << " Not handled";
      break;
  }
}

}  // namespace media
