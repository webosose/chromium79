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

#ifndef NEVA_PAL_MEDIA_SERVICE_PC_MEDIAPLAYER_STUB_H_
#define NEVA_PAL_MEDIA_SERVICE_PC_MEDIAPLAYER_STUB_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "neva/pal_media_service/mediaplayer_base_impl.h"
#include "neva/pal_media_service/media_track_info.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
}

namespace gfx {
class RectF;
}

namespace pal_media {

class MediaPlayerStub : public base::SupportsWeakPtr<MediaPlayerStub>,
                        public MediaPlayerBaseImpl {
 public:
  // Constructs a RendererMediaPlayerManager object for the |render_frame|.
  MediaPlayerStub() {}
  ~MediaPlayerStub() override {}

  // media::RendererMediaBuiltinPlayerManagerInterface implementation
  // Initializes a MediaPlayerAndroid object in browser process.
  void Initialize(const bool is_video,
                  const double current_time,
                  const std::string& app_id,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& media_option,
                  const std::string& custom_option) override {}

  // Starts the player.
  void Start() override {}

  // Pauses the player.
  void Pause() override {}

  // Performs seek on the player.
  void Seek(const base::TimeDelta time) override {}

  void SetRate(double rate) override {}

  // Sets the player volume.
  void SetVolume(double volume) override {}

  // Sets the poster image.
  void SetPoster(const GURL& poster) override {}

  // bool IsSupportedBackwardTrickPlay() override {}
  void SetPreload(mojom::Preload preload) override {}
  // TODO(wanchang): fix the type of preload

  void IsPreloadable(const std::string& content_media_option,
                     IsPreloadableCallback callback) override {}
  void HasVideo(HasVideoCallback callback) override {}
  void HasAudio(HasAudioCallback callback) override {}
  void SelectTrack(const mojom::MediaTrackType type,
                   const std::string& id) override {}
  // gfx::Size NaturalVideoSize() override;
  // double Duration() override;
  // double CurrentTime() override;
  void SwitchToAutoLayout() override {}
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override {}
  void UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) override {}
  void MediaId(MediaIdCallback callback) override {}
  void HasAudioFocus(HasAudioFocusCallback callback) override {}
  void SetAudioFocus(bool focus) override {}
  void HasVisibility(HasVisibilityCallback callback) override {}
  void SetVisibility(bool) override {}
  void Suspend(mojom::SuspendReason reason) override {}
  void Resume() override {}
  void RequireMediaResource(RequireMediaResourceCallback callbck) override {}
  void IsRecoverableOnResume(IsRecoverableOnResumeCallback callback) override {}
  void SetDisableAudio(bool) override {}
  // end of media::RendererMediaBuiltinPlayerManagerInterface
  //-----------------------------------------------------------------

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaPlayerStub);
};

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_PC_MEDIAPLAYER_STUB_H_
