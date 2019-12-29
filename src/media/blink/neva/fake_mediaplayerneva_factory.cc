// Copyright 2017-2019 LG Electronics, Inc.
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

#include "media/blink/neva/mediaplayerneva_factory.h"

#include "base/command_line.h"
#include "media/base/media_switches_neva.h"
#include "media/blink/neva/fake_url_mediaplayer.h"
#include "net/base/mime_util.h"

namespace media {

pal_media::mojom::MediaPlayerType MediaPlayerNevaFactory::GetMediaPlayerType(
    const std::string& mime_type) {
  return pal_media::mojom::MediaPlayerType::kMediaPlayerTypeUMS;
}

MediaPlayerNeva* MediaPlayerNevaFactory::CreateMediaPlayerNeva(
    MediaPlayerNevaClient* client,
    const pal_media::mojom::MediaPlayerType media_player_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner) {
  return new FakeURLMediaPlayer(client, main_task_runner);
}

}  // namespace media
