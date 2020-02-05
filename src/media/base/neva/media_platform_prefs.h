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

#include "base/memory/singleton.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT MediaPlatformPrefs {
 public:
  static MediaPlatformPrefs* Get();

  /* The "mediaExtension" in app_info.json is passed as below
  "mediaExtension":{"mse":{"enableAV1":true,
                           "disableVideoIntrinsicSize":true,
                           "maxAudioSourceBuffer":15,
                           "maxVideoSourceBuffer":100},
                    "ums":{"fixedAspectRatio":true }
                   }
  */
  void Update(const std::string& pref_json);

  bool IsDisableVideoIntrinsicSizeForMSE();

 private:
  friend struct base::DefaultSingletonTraits<MediaPlatformPrefs>;

  MediaPlatformPrefs();
  virtual ~MediaPlatformPrefs();

  // MSE Preferences
  bool mse_disable_video_intrinsic_size_ = false;
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_MEDIA_PLATFORM_PREFS_H_
