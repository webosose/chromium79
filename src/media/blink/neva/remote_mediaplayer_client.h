// Copyright 2019-2020 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_REMOTE_MEDIAPLAYER_CLIENT_H_
#define MEDIA_BLINK_NEVA_REMOTE_MEDIAPLAYER_CLIENT_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/renderer/render_frame_observer.h"
#include "media/blink/neva/media_player_neva_interface.h"
#include "media/blink/neva/webos/webos_mediaclient.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "neva/pal_media_service/public/mojom/constants.mojom.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "third_party/blink/public/platform/web_string.h"
#include "url/gurl.h"

namespace gfx {
class RectF;
}

namespace media {

class RemoteMediaPlayerClient
    : public base::SupportsWeakPtr<RemoteMediaPlayerClient>,
      public pal_media::mojom::MediaPlayerListener,
      public MediaPlayerNeva {
 public:
  explicit RemoteMediaPlayerClient(
      MediaPlayerNevaClient*,
      pal_media::mojom::MediaPlayerType,
      const scoped_refptr<base::SingleThreadTaskRunner>&,
      const std::string&);
  ~RemoteMediaPlayerClient() override;

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
  void Seek(const base::TimeDelta& time) override;

  void SetRate(double rate) override;

  // Sets the player volume.
  void SetVolume(double volume) override;

  // Sets the poster image.
  void SetPoster(const GURL& poster) override;

  void SetPreload(Preload preload) override;
  bool IsPreloadable(const std::string& content_media_option) override;
  bool HasVideo() override;
  bool HasAudio() override;
  bool SelectTrack(const MediaTrackType type, const std::string& id) override;
  void SwitchToAutoLayout() override;
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override;
  bool UsesIntrinsicSize() const override;
  std::string MediaId() const override;
  bool HasAudioFocus() const override;
  void SetAudioFocus(bool focus) override;
  bool HasVisibility() const override;
  void SetVisibility(bool) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  bool RequireMediaResource() const override;
  bool IsRecoverableOnResume() const override;
  void SetDisableAudio(bool) override;
  void SetMediaLayerId(const std::string& media_layer_id) override;
  media::Ranges<base::TimeDelta> GetBufferedTimeRanges() const override;
  bool Send(const std::string& message) const override;

  // pal_media::mojom::MediaPlayerListener
  void OnMediaPlayerPlay() override;
  void OnMediaPlayerPause() override;
  void OnPlaybackComplete() override;
  void OnMediaError(int error) override;
  void OnSeekComplete(base::TimeDelta current_time) override;
  void OnMediaMetadataChanged(base::TimeDelta duration,
                              uint32_t width,
                              uint32_t height,
                              bool success) override;
  void OnLoadComplete() override;
  void OnVideoSizeChanged(uint32_t width, uint32_t height) override;
  void OnCustomMessage(const pal_media::mojom::MediaEventType,
                       const std::string& detail) override;
  void OnBufferingStateChanged(BufferingState buffering_state) override;
  void OnTimeUpdate(base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks) override;
  void OnAudioTracksUpdated(
      const std::vector<media::MediaTrackInfo>& audio_track_info) override;
  void OnAudioFocusChanged() override;
  void OnActiveRegionChanged(const gfx::Rect&) override;
  // end of pal_media::mojom::MediaPlayerListener

 private:
  void OnSubscribeRespond(
      pal_media::mojom::MediaPlayerListenerAssociatedRequest request);

  MediaPlayerNevaClient* client_;
  base::TimeDelta paused_time_;

  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;

  base::RepeatingTimer time_update_timer_;

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  std::string identifier_;
  mojo::AssociatedBinding<pal_media::mojom::MediaPlayerListener>
      listener_binding_;
  pal_media::mojom::MediaPlayerPtr media_player_;

  base::Optional<bool> has_video_;
  base::Optional<bool> has_audio_;
  base::Optional<bool> uses_intrinsic_size_;
  base::Optional<bool> is_recoverable_on_resume_;
  base::Optional<bool> has_audio_focus_;
  base::Optional<bool> has_visibility_;

  DISALLOW_COPY_AND_ASSIGN(RemoteMediaPlayerClient);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_REMOTE_MEDIAPLAYER_CLIENT_H_
