// Copyright 2019 LG Electronics, Inc.
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

#include "neva/pal_media_service/pal_media_player_factory.h"

#include "neva/pal_media_service/mediaplayer_stub.h"

#include <memory>
#include <mutex>

namespace pal_media {
namespace {

std::once_flag platform_media_factory_flag;

}  // namespace

// static
PalMediaPlayerFactory* PalMediaPlayerFactory::Get() {
  static PalMediaPlayerFactory* platform_media_factory;
  std::call_once(platform_media_factory_flag, []() {
    platform_media_factory = new PalMediaPlayerFactory();
  });
  return platform_media_factory;
}

#if !defined(OS_WEBOS)
std::unique_ptr<pal_media::mojom::MediaPlayer>
PalMediaPlayerFactory::CreateMediaPlayer(
    mojom::MediaPlayerType media_player_type,
    const std::string& app_id) {
  return std::make_unique<MediaPlayerStub>();
}
#endif

PalMediaPlayerFactory::PalMediaPlayerFactory() {}

}  // namespace pal_media
