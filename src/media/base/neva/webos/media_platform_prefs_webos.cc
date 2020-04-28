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

#include "media/base/neva/webos/media_platform_prefs_webos.h"

namespace media {

namespace {
const char kMSE[]                       = "mse";
const char kDisableVideoIntrinsicSize[] = "disableVideoIntrinsicSize";
}

//  static
MediaPlatformPrefs* MediaPlatformPrefs::Get() {
  return MediaPlatformPrefsWebOS::Get();
}

//  static
MediaPlatformPrefsWebOS* MediaPlatformPrefsWebOS::Get() {
  return base::Singleton<MediaPlatformPrefsWebOS>::get();
}

MediaPlatformPrefsWebOS::MediaPlatformPrefsWebOS() = default;
MediaPlatformPrefsWebOS::~MediaPlatformPrefsWebOS() = default;

void MediaPlatformPrefsWebOS::Update(const std::string& pref_json) {
  Json::Value preferences;
  Json::Reader reader;

  if (!reader.parse(pref_json, preferences))
    return;

  // MSE Preferences
  if (preferences.isMember(kMSE)) {
    if (preferences[kMSE].isMember(kDisableVideoIntrinsicSize)) {
      if (mse_disable_video_intrinsic_size_ !=
          preferences[kMSE][kDisableVideoIntrinsicSize].asBool()) {
        mse_disable_video_intrinsic_size_ =
            preferences[kMSE][kDisableVideoIntrinsicSize].asBool();
        VLOG(1) << __func__ << " : mse_disable_video_intrinsic_size_"
                << mse_disable_video_intrinsic_size_;
      }
    }
  }
}

}  // namespace media
