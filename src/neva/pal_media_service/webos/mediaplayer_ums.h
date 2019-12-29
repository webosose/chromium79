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

#ifndef NEVA_PAL_MEDIA_SERVICE_WEBOS_MEDIA_PLAYER_UMS_H_
#define NEVA_PAL_MEDIA_SERVICE_WEBOS_MEDIA_PLAYER_UMS_H_

#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "neva/pal_media_service/mediaplayer_base_impl.h"
#include "neva/pal_media_service/media_track_info.h"
#include "neva/pal_media_service/webos/webos_mediaclient.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
}

namespace gfx {
class RectF;
}

namespace pal_media {

class MediaPlayerUMS : public base::SupportsWeakPtr<MediaPlayerUMS>,
                       public MediaPlayerBaseImpl {
 public:
  MediaPlayerUMS();
  ~MediaPlayerUMS() override;

  void Initialize(const bool is_video,
                  const double current_time,
                  const std::string& app_id,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& media_option,
                  const std::string& custom_option) override;

  // Starts the player.
  void Start() override;

  // Pauses the player.
  void Pause() override;

  // Performs seek on the player.
  void Seek(const base::TimeDelta time) override;

  void SetRate(double rate) override;

  // Sets the player volume.
  void SetVolume(double volume) override;

  // Sets the poster image.
  void SetPoster(const GURL& poster) override;

  void SetPreload(mojom::Preload preload) override;

  void IsPreloadable(const std::string& content_media_option,
                     IsPreloadableCallback callback) override;
  void HasVideo(HasVideoCallback callback) override;
  void HasAudio(HasAudioCallback callback) override;
  void SelectTrack(const mojom::MediaTrackType type,
                   const std::string& id) override;
  void SwitchToAutoLayout() override;
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override;
  void UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) override;
  void MediaId(MediaIdCallback callback) override;
  void HasAudioFocus(HasAudioFocusCallback callback) override;
  void SetAudioFocus(bool focus) override;
  void HasVisibility(HasVisibilityCallback callback) override;
  void SetVisibility(bool) override;
  void Suspend(mojom::SuspendReason reason) override;
  void Resume() override;
  void RequireMediaResource(RequireMediaResourceCallback callbck) override;
  void IsRecoverableOnResume(IsRecoverableOnResumeCallback callback) override;
  void SetDisableAudio(bool) override;
  //-----------------------------------------------------------------

 private:
  void OnStreamEnded();
  void ActiveRegionChanged(const gfx::Rect& active_region);
  void OnSeekDone(media::PipelineStatus status);
  void OnBufferingState(WebOSMediaClient::BufferingState buffering_state);
  void OnVideoSizeChange();
  void OnVideoDisplayWindowChange();

  base::TimeDelta GetCurrentTime();
  void OnTimeUpdateTimerFired();

  std::unique_ptr<WebOSMediaClient> umedia_client_;
  bool paused_;
  base::TimeDelta paused_time_;
  double playback_rate_;
  bool is_suspended_;

  bool fullscreen_;
  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;
  bool active_video_region_changed_;

  bool is_video_offscreen_;

  base::RepeatingTimer time_update_timer_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  mojo::AssociatedInterfacePtrSet<mojom::MediaPlayerListener> listeners_;
  mojo::BindingSet<mojom::MediaPlayer> bindings_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerUMS);
  base::WeakPtrFactory<MediaPlayerUMS> weak_factory_;
};

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_NEVA_WEBOS_MEDIAPLAYER_UMS_H_
