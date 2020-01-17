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

#ifndef NEVA_PAL_MEDIA_SERVICE_WEBOS_MEDIAPLAYER_CAMERA_H_
#define NEVA_PAL_MEDIA_SERVICE_WEBOS_MEDIAPLAYER_CAMERA_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/task/post_task.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "neva/pal_media_service/mediaplayer_base_impl.h"
#include "neva/pal_media_service/webos/webos_mediaclient.h"

class GURL;

namespace pal_media {

class MediaPlayerCamera : public base::SupportsWeakPtr<MediaPlayerCamera>,
                          public MediaPlayerBaseImpl {
 public:
  explicit MediaPlayerCamera(const std::string& app_id);
  ~MediaPlayerCamera() override;

  // media::MediaPlayerNeva implementation
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
  void Seek(const base::TimeDelta time) override {}
  // Sets the player volume.
  void SetVolume(double volume) override {}
  // Sets the poster image.
  void SetPoster(const GURL& poster) override {}

  void SetRate(double rate) override {}
  void SetPreload(mojom::Preload preload) override {}
  void IsPreloadable(const std::string& content_media_option,
                     IsPreloadableCallback callback) override;
  void HasVideo(HasVideoCallback callback) override;
  void HasAudio(HasAudioCallback callback) override;
  void SelectTrack(const media::MediaTrackType type,
                   const std::string& id) override;
  void SwitchToAutoLayout() override;
  void SetDisplayWindow(const gfx::Rect& out_rect,
                        const gfx::Rect& in_rect,
                        bool full_screen,
                        bool forced) override;
  void UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) override;
  void MediaId(MediaIdCallback callback) override;
  void Suspend(mojom::SuspendReason reason) override {}
  void Resume() override {}
  void IsRecoverableOnResume(IsRecoverableOnResumeCallback callback) override;
  void HasAudioFocus(HasAudioFocusCallback callback) override;
  void SetAudioFocus(bool focus) override {}
  void HasVisibility(HasVisibilityCallback callback) override;
  void SetVisibility(bool) override;
  void RequireMediaResource(RequireMediaResourceCallback callback) override;
  void SetDisableAudio(bool) override {}
  void SetMediaLayerId(const std::string& media_layer_id) override;

 private:
  void OnPlaybackStateChanged(bool playing);
  void OnStreamEnded();
  void OnSeekDone(media::PipelineStatus status) {}
  void OnBufferingState(WebOSMediaClient::BufferingState buffering_state);
  void OnDurationChange() {}
  void OnVideoSizeChange();
  void OnVideoDisplayWindowChange();
  void OnUpdateUMSInfo(const std::string& detail);
  void OnAudioFocusChanged() {}
  void OnActiveRegionChanged(const gfx::Rect& active_region);

  base::TimeDelta GetCurrentTime();
  void OnTimeUpdateTimerFired();

  std::unique_ptr<WebOSMediaClient> umedia_client_;

  std::string app_id_;
  GURL url_;
  std::string mime_type_;
  std::string camera_id_;

  double playback_rate_;

  bool is_video_offscreen_;
  bool fullscreen_;
  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;

  base::RepeatingTimer time_update_timer_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerCamera);
  base::WeakPtrFactory<MediaPlayerCamera> weak_factory_;
};

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_WEBOS_MEDIAPLAYER_CAMERA_H_
