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
#include "neva/pal_media_service/webos/mediaplayer_ums.h"

#if defined(USE_GST_MEDIA)
#include "neva/pal_media_service/webos/mediaplayer_camera.h"
#endif

#include <memory>

namespace pal_media {

std::unique_ptr<pal_media::mojom::MediaPlayer>
PalMediaPlayerFactory::CreateMediaPlayer(
    mojom::MediaPlayerType media_player_type) {
  switch (media_player_type) {
#if defined(USE_GST_MEDIA)
    case mojom::MediaPlayerType::kMediaPlayerTypeCamera:
      return std::make_unique<MediaPlayerCamera>();
      break;
#endif
    case mojom::MediaPlayerType::kMediaPlayerTypeUMS:
      return std::make_unique<MediaPlayerUMS>();
      break;
    default:
      break;
  }
  return nullptr;
}

}  // namespace pal_media
