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

#ifndef NEVA_PAL_MEDIA_SERVICE_MEDIAPLAYER_BASE_IMPL_H_
#define NEVA_PAL_MEDIA_SERVICE_MEDIAPLAYER_BASE_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_task_traits.h"
#include "media/base/neva/media_track_info.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "neva/pal_media_service/webos/webos_mediaclient.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
}

namespace gfx {
class RectF;
}

namespace pal_media {

static const int kTimeUpdateInterval = 100;

class MediaPlayerBaseImpl : public mojom::MediaPlayer {
 public:
  MediaPlayerBaseImpl();
  ~MediaPlayerBaseImpl() override;

  void Subscribe(SubscribeCallback callback) override;

  void Initialize(const bool is_video,
                  const double current_time,
                  const std::string& app_id,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& media_option,
                  const std::string& custom_option) override = 0;

  // Starts the player.
  void Start() override = 0;

  // Pauses the player.
  void Pause() override = 0;

  // Performs seek on the player.
  void Seek(const base::TimeDelta time) override = 0;

  void SetRate(double rate) override = 0;

  // Sets the player volume.
  void SetVolume(double volume) override = 0;

  // Sets the poster image.
  void SetPoster(const GURL& poster) override = 0;

  void SetPreload(mojom::Preload preload) override = 0;

  void IsPreloadable(const std::string& content_media_option,
                     IsPreloadableCallback callback) override = 0;
  void HasVideo(HasVideoCallback callback) override = 0;
  void HasAudio(HasAudioCallback callback) override = 0;
  void SelectTrack(const media::MediaTrackType type,
                   const std::string& id) override = 0;
  void SwitchToAutoLayout() override = 0;
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override = 0;
  void UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) override = 0;
  void MediaId(MediaIdCallback callback) override = 0;
  void HasAudioFocus(HasAudioFocusCallback callback) override = 0;
  void SetAudioFocus(bool focus) override = 0;
  void HasVisibility(HasVisibilityCallback callback) override = 0;
  void SetVisibility(bool) override = 0;
  void Suspend(mojom::SuspendReason reason) override = 0;
  void Resume() override = 0;
  void IsRecoverableOnResume(IsRecoverableOnResumeCallback callback) override =
      0;
  void SetDisableAudio(bool) override = 0;
  void SetMediaLayerId(const std::string& media_layer_id) override = 0;
  void RequireMediaResource(RequireMediaResourceCallback callback) override = 0;

  //-----------------------------------------------------------------
  // umediaclient callbacks
  void OnPlaybackStateChanged(bool playing);
  void OnStreamEnded();
  void OnSeekDone(media::PipelineStatus status,
                  const base::TimeDelta& current_time);
  void OnError(media::PipelineStatus error);
  void OnBufferingState(WebOSMediaClient::BufferingState buffering_state,
                        double duration,
                        const gfx::Size& natural_video_size);
  void OnDurationChange();
  void OnVideoSizeChange(const gfx::Size& natural_video_size);
  void OnAddAudioTrack(
      const std::vector<media::MediaTrackInfo>& audio_track_info);
  void OnAddVideoTrack(const std::string& id,
                       const std::string& kind,
                       const std::string& language,
                       bool enabled);
  void UpdateUMSInfo(const std::string& detail);
  void OnAudioFocusChanged();
  void ActiveRegionChanged(const gfx::Rect& active_region);
  void OnWaitingForDecryptionKey();
  void OnEncryptedMediaInitData(const std::string& init_data_type,
                                const std::vector<uint8_t>& init_data);

  void OnTimeUpdateTimerFired(const base::TimeDelta& current_time);

  void OnMediaMetadataChanged(const base::TimeDelta& duration,
                              int width,
                              int height,
                              bool success);
  void OnLoadComplete();
  void OnCustomMessage(const pal_media::mojom::MediaEventType,
                       const std::string& detail);

 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  mojo::AssociatedInterfacePtrSet<mojom::MediaPlayerListener> listeners_;
  mojo::BindingSet<mojom::MediaPlayer> bindings_;

  base::WeakPtrFactory<MediaPlayerBaseImpl> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBaseImpl);
};

class MediaPlayerProviderImpl : public mojom::MediaPlayerProvider {
 public:
  MediaPlayerProviderImpl();
  ~MediaPlayerProviderImpl() override;

  void AddBinding(mojom::MediaPlayerProviderRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  void GetMediaPlayer(mojom::MediaPlayerType media_player_type,
                      const std::string& app_id,
                      mojom::MediaPlayerRequest request) override;

 private:
  mojo::BindingSet<mojom::MediaPlayerProvider> bindings_;
  DISALLOW_COPY_AND_ASSIGN(MediaPlayerProviderImpl);
};

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_NEVA_MEDIAPLAYER_BASE_IMPL_H_
