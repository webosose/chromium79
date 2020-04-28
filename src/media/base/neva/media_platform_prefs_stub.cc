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

#include "media/base/neva/media_platform_prefs_stub.h"

#include "base/lazy_instance.h"

namespace media {

//  static
MediaPlatformPrefs* MediaPlatformPrefs::Get() {
  return MediaPlatformPrefsStub::Get();
}

//  static
MediaPlatformPrefsStub* MediaPlatformPrefsStub::Get() {
  return base::Singleton<MediaPlatformPrefsStub>::get();
}

MediaPlatformPrefsStub::MediaPlatformPrefsStub() = default;
MediaPlatformPrefsStub::~MediaPlatformPrefsStub() = default;

void MediaPlatformPrefsStub::Update(const std::string& pref_json) {
  NOTREACHED();
}

}  // namespace media
