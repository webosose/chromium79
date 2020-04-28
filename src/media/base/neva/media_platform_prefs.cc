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

#include "media/base/neva/media_platform_prefs.h"

#include "base/logging_pmlog.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

namespace {
const char kMaxWidth[]     = "maxWidth";
const char kMaxHeight[]    = "maxHeight";
const char kMaxFrameRate[] = "maxFrameRate";
const char kMaxBitRate[]   = "maxBitRate";
const char kChannels[]     = "channels";
}

MediaPlatformPrefs::MediaPlatformPrefs() = default;

MediaPlatformPrefs::~MediaPlatformPrefs() = default;

bool MediaPlatformPrefs::IsDisableVideoIntrinsicSizeForMSE() {
  return mse_disable_video_intrinsic_size_;
}

base::Optional<MediaTypeRestriction>
MediaPlatformPrefs::GetMediaRestriction(const std::string& type) {
  if (media_codec_capability_[type].empty())
    return base::nullopt;

  MediaTypeRestriction restriction = MediaTypeRestriction(
      media_codec_capability_[type][kMaxWidth].asInt(),
      media_codec_capability_[type][kMaxHeight].asInt(),
      media_codec_capability_[type][kMaxFrameRate].asInt(),
      media_codec_capability_[type][kMaxBitRate].asInt() * 1024 * 1024,
      media_codec_capability_[type][kChannels].asInt());

  return restriction;
}

void MediaPlatformPrefs::SetMediaCodecCapability(
    const std::string& media_codec_capability) {
  Json::Value codec_capability;
  Json::Reader reader;

  if (!media_codec_capability_.isNull())
    return;

  if (!reader.parse(media_codec_capability, codec_capability))
    return;

  Json::Value videoCodecs = codec_capability["videoCodecs"];
  Json::Value audioCodecs = codec_capability["audioCodecs"];

  for (Json::Value::iterator iter = videoCodecs.begin();
       iter != videoCodecs.end(); iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "H.264") {
        media_codec_capability_["H264"] = *iter;
        media_codec_capability_["video/mp4"] = *iter;
      } else if ((*iter)["name"].asString() == "H.265") {
        media_codec_capability_["H265"] = *iter;
      } else if ((*iter)["name"].asString() == "HEVC") {
        media_codec_capability_["HEVC"] = *iter;
        media_codec_capability_["video/hvc1"] = *iter;
      } else if ((*iter)["name"].asString() == "VP8") {
        media_codec_capability_["VP8"] = *iter;
        media_codec_capability_["video/vp8"] = *iter;
      } else if ((*iter)["name"].asString() == "VP9") {
        media_codec_capability_["VP9"] = *iter;
        media_codec_capability_["video/vp9"] = *iter;
      } else if ((*iter)["name"].asString() == "AV1") {
        media_codec_capability_["AV1"] = *iter;
      }
    }
  }

  for (Json::Value::iterator iter = audioCodecs.begin();
       iter != audioCodecs.end(); iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "AAC")
        media_codec_capability_["audio/mp4"] = *iter;
    }
  }
}

}  // namespace media
