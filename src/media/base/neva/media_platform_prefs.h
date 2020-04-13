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

#ifndef MEDIA_BASE_NEVA_MEDIA_PLATFORM_PREFS_H_
#define MEDIA_BASE_NEVA_MEDIA_PLATFORM_PREFS_H_

#include <string>

#include "base/optional.h"
#include "media/base/media_export.h"
#include "media/base/neva/media_type_restriction.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#if defined(USE_NEVA_WEBRTC)
#include "media/base/video_codecs.h"
#endif

namespace media {

class MEDIA_EXPORT MediaPlatformPrefs {
 public:
  // static
  static MediaPlatformPrefs* Get();

  /* The "mediaExtension" in app_info.json is passed as below
  "mediaExtension":{"mse":{"enableAV1":true,
                           "disableVideoIntrinsicSize":true,
                           "maxAudioSourceBuffer":15,
                           "maxVideoSourceBuffer":100},
                    "ums":{"fixedAspectRatio":true }
                   }
  */
  virtual void Update(const std::string& pref_json) = 0;

  bool IsDisableVideoIntrinsicSizeForMSE();

  base::Optional<MediaTypeRestriction> GetMediaRestriction(
      const std::string& type);
  void SetMediaCodecCapability(const std::string& media_codec_capability);

#if defined(USE_NEVA_WEBRTC)
  // TODO: Further future refactoring of audio/video codec type supported
  // and MediaTypeRestriction support is planned as mentioned in PLAT-106543
  bool IsCodecSupported(VideoCodec codec);
#endif

 protected:
  MediaPlatformPrefs();
  virtual ~MediaPlatformPrefs();

  // MSE Preferences
  bool mse_disable_video_intrinsic_size_ = false;

  Json::Value media_codec_capability_;
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_MEDIA_PLATFORM_PREFS_H_
