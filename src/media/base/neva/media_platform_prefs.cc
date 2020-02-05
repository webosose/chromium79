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

#include "base/lazy_instance.h"
#include "base/logging_pmlog.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

//  static
MediaPlatformPrefs* MediaPlatformPrefs::Get() {
  return base::Singleton<MediaPlatformPrefs>::get();
}

MediaPlatformPrefs::MediaPlatformPrefs() {}
MediaPlatformPrefs::~MediaPlatformPrefs() {}

void MediaPlatformPrefs::Update(const std::string& pref_json) {
  Json::Value preferences;
  Json::Reader reader;

  if (!reader.parse(pref_json, preferences))
    return;

  // MSE Preferences
  if (preferences.isMember("mse")) {
    if (preferences["mse"].isMember("disableVideoIntrinsicSize")) {
      if (mse_disable_video_intrinsic_size_ !=
          preferences["mse"]["disableVideoIntrinsicSize"].asBool()) {
        mse_disable_video_intrinsic_size_ =
            preferences["mse"]["disableVideoIntrinsicSize"].asBool();
        VLOG(1) << __func__ << " : mse_disable_video_intrinsic_size_"
                << mse_disable_video_intrinsic_size_;
      }
    }
  }
}

bool MediaPlatformPrefs::IsDisableVideoIntrinsicSizeForMSE() {
  return mse_disable_video_intrinsic_size_;
}

}  // namespace media
