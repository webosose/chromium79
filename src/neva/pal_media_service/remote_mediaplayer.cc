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

#include "neva/pal_media_service/remote_mediaplayer.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "media/blink/neva/mediaplayerneva_factory.h"
#include "ui/gfx/geometry/rect_f.h"

namespace pal_media {

RemoteMediaPlayer::RemoteMediaPlayer(mojom::MediaPlayerType media_player_type,
                                     const std::string& app_id) {
  // TODO(neva): It was designed that media players in MediaPlayerNeva are
  // working with main thread(main task runner) in renderer process. In pal
  // media service, this rule is violated. We call RemoteMediaPlayer ->
  // MediaPlayerNeva by 'unknown' thread, and run callback in created task
  // runner. So we need to make sure right way.
  media_player_neva_.reset(media::MediaPlayerNevaFactory::CreateMediaPlayerNeva(
      this, media_player_type,
      base::CreateSingleThreadTaskRunner({base::ThreadPool()}), app_id, true));
}

RemoteMediaPlayer::~RemoteMediaPlayer() {}

void RemoteMediaPlayer::Subscribe(SubscribeCallback callback) {
  mojom::MediaPlayerListenerAssociatedPtr listener;
  mojom::MediaPlayerListenerAssociatedRequest request =
      mojo::MakeRequest(&(listener));
  listeners_.AddPtr(std::move(listener));
  std::move(callback).Run(std::move(request));
}

void RemoteMediaPlayer::Initialize(const bool is_video,
                                   const double current_time,
                                   const std::string& app_id,
                                   const std::string& url,
                                   const std::string& mime,
                                   const std::string& referrer,
                                   const std::string& user_agent,
                                   const std::string& cookies,
                                   const std::string& media_option,
                                   const std::string& custom_option) {
  media_player_neva_->Initialize(is_video, current_time, app_id, url, mime,
                                 referrer, user_agent, cookies, media_option,
                                 custom_option);
}

void RemoteMediaPlayer::Start() {
  media_player_neva_->Start();
}

void RemoteMediaPlayer::Pause() {
  media_player_neva_->Pause();
}

void RemoteMediaPlayer::Seek(const base::TimeDelta time) {
  media_player_neva_->Seek(time);
}

void RemoteMediaPlayer::SetRate(double rate) {
  media_player_neva_->SetRate(rate);
}

void RemoteMediaPlayer::SetVolume(double volume) {
  media_player_neva_->SetVolume(volume);
}

void RemoteMediaPlayer::SetPoster(const GURL& poster) {
  media_player_neva_->SetPoster(poster);
}

void RemoteMediaPlayer::SetPreload(mojom::Preload preload) {
  media::MediaPlayerNeva::Preload converted_value;

  switch (preload) {
    case mojom::Preload::kPreloadNone:
      converted_value = media::MediaPlayerNeva::Preload::PreloadNone;
      break;
    case mojom::Preload::kPreloadMetaData:
      converted_value = media::MediaPlayerNeva::Preload::PreloadMetaData;
      break;
    case mojom::Preload::kPreloadAuto:
      converted_value = media::MediaPlayerNeva::Preload::PreloadAuto;
      break;
  }

  media_player_neva_->SetPreload(converted_value);
}

void RemoteMediaPlayer::IsPreloadable(const std::string& content_media_option,
                                      IsPreloadableCallback callback) {
  std::move(callback).Run(
      media_player_neva_->IsPreloadable(content_media_option));
}

void RemoteMediaPlayer::HasVideo(HasVideoCallback callback) {
  std::move(callback).Run(media_player_neva_->HasVideo());
}

void RemoteMediaPlayer::HasAudio(HasAudioCallback callback) {
  std::move(callback).Run(media_player_neva_->HasAudio());
}

void RemoteMediaPlayer::SelectTrack(const media::MediaTrackType type,
                                    const std::string& id) {
  media_player_neva_->SelectTrack(type, id);
}

void RemoteMediaPlayer::SwitchToAutoLayout() {
  media_player_neva_->SwitchToAutoLayout();
}

void RemoteMediaPlayer::SetDisplayWindow(const gfx::Rect& out_rect,
                                         const gfx::Rect& in_rect,
                                         bool full_screen,
                                         bool forced) {
  media_player_neva_->SetDisplayWindow(out_rect, in_rect, full_screen, forced);
}

void RemoteMediaPlayer::UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) {
  std::move(callback).Run(media_player_neva_->UsesIntrinsicSize());
}

void RemoteMediaPlayer::MediaId(MediaIdCallback callback) {
  std::move(callback).Run(media_player_neva_->MediaId());
}

void RemoteMediaPlayer::HasAudioFocus(HasAudioFocusCallback callback) {
  std::move(callback).Run(media_player_neva_->HasAudioFocus());
}

void RemoteMediaPlayer::SetAudioFocus(bool focus) {
  media_player_neva_->SetAudioFocus(focus);
}

void RemoteMediaPlayer::HasVisibility(HasVisibilityCallback callback) {
  std::move(callback).Run(media_player_neva_->HasVisibility());
}

void RemoteMediaPlayer::SetVisibility(bool visibility) {
  media_player_neva_->SetVisibility(visibility);
}

void RemoteMediaPlayer::Suspend(mojom::SuspendReason reason) {
  media::SuspendReason converted = media::SuspendReason::BACKGROUNDED;
  switch (reason) {
    case mojom::SuspendReason::kBackground:
      converted = media::SuspendReason::BACKGROUNDED;
      break;
    case mojom::SuspendReason::kSuspendedByPolicy:
      converted = media::SuspendReason::SUSPENDED_BY_POLICY;
      break;
  }
  media_player_neva_->Suspend(converted);
}

void RemoteMediaPlayer::Resume() {
  media_player_neva_->Resume();
}

void RemoteMediaPlayer::RequireMediaResource(
    RequireMediaResourceCallback callback) {
  std::move(callback).Run(media_player_neva_->RequireMediaResource());
}

void RemoteMediaPlayer::SetMediaLayerId(const std::string& media_layer_id) {
  media_player_neva_->SetMediaLayerId(media_layer_id);
}

void RemoteMediaPlayer::IsRecoverableOnResume(
    IsRecoverableOnResumeCallback callback) {
  std::move(callback).Run(media_player_neva_->IsRecoverableOnResume());
}

void RemoteMediaPlayer::SetDisableAudio(bool disable) {
  media_player_neva_->SetDisableAudio(disable);
}

void RemoteMediaPlayer::GetBufferedTimeRanges(
    GetBufferedTimeRangesCallback callback) {
  std::vector<pal_media::mojom::TimeDeltaPairPtr> vector_ranges;
  media::Ranges<base::TimeDelta> ranges =
      media_player_neva_->GetBufferedTimeRanges();

  for (size_t i = 0; i < ranges.size(); i++) {
    vector_ranges.push_back(
        mojom::TimeDeltaPair(ranges.start(i), ranges.end(i)).Clone());
  }

  std::move(callback).Run(std::move(vector_ranges));
}

void RemoteMediaPlayer::OnMediaMetadataChanged(base::TimeDelta duration,
                                               int width,
                                               int height,
                                               bool success) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnMediaMetadataChanged(duration, width, height, success);
  });
}

void RemoteMediaPlayer::OnLoadComplete() {
  listeners_.ForAllPtrs(
      [](mojom::MediaPlayerListener* listener) { listener->OnLoadComplete(); });
}

void RemoteMediaPlayer::OnPlaybackComplete() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnPlaybackComplete();
  });
}

void RemoteMediaPlayer::OnSeekComplete(const base::TimeDelta& current_time) {
  listeners_.ForAllPtrs([&current_time](mojom::MediaPlayerListener* listener) {
    listener->OnSeekComplete(current_time);
  });
}

void RemoteMediaPlayer::OnMediaError(int error) {
  listeners_.ForAllPtrs([&error](mojom::MediaPlayerListener* listener) {
    listener->OnMediaError(error);
  });
}

void RemoteMediaPlayer::OnVideoSizeChanged(int width, int height) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnVideoSizeChanged(width, height);
  });
}

void RemoteMediaPlayer::OnMediaPlayerPlay() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnMediaPlayerPlay();
  });
}

void RemoteMediaPlayer::OnMediaPlayerPause() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnMediaPlayerPause();
  });
}

void RemoteMediaPlayer::OnCustomMessage(
    const blink::WebMediaPlayer::MediaEventType media_event_type,
    const std::string& detail) {
  pal_media::mojom::MediaEventType converted_type =
      pal_media::mojom::MediaEventType::kMediaEventNone;

  switch (media_event_type) {
    case blink::WebMediaPlayer::kMediaEventNone:
      converted_type = pal_media::mojom::MediaEventType::kMediaEventNone;
      break;
    case blink::WebMediaPlayer::kMediaEventUpdateUMSMediaInfo:
      converted_type =
          pal_media::mojom::MediaEventType::kMediaEventUpdateUMSMediaInfo;
      break;
    case blink::WebMediaPlayer::kMediaEventBroadcastErrorMsg:
      converted_type =
          pal_media::mojom::MediaEventType::kMediaEventBroadcastErrorMsg;
      break;
    case blink::WebMediaPlayer::kMediaEventDvrErrorMsg:
      converted_type = pal_media::mojom::MediaEventType::kMediaEventDvrErrorMsg;
      break;
    case blink::WebMediaPlayer::kMediaEventUpdateCameraState:
      converted_type =
          pal_media::mojom::MediaEventType::kMediaEventUpdateCameraState;
      break;
    case blink::WebMediaPlayer::kMediaEventPipelineStarted:
      converted_type =
          pal_media::mojom::MediaEventType::kMediaEventPipelineStarted;
      break;
  }

  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnCustomMessage(converted_type, detail);
  });
}

void RemoteMediaPlayer::OnBufferingUpdate(int percentage) {
  listeners_.ForAllPtrs([&percentage](mojom::MediaPlayerListener* listener) {
    listener->OnBufferingUpdate(percentage);
  });
}

void RemoteMediaPlayer::OnAudioTracksUpdated(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  listeners_.ForAllPtrs(
      [&audio_track_info](mojom::MediaPlayerListener* listener) {
        listener->OnAudioTracksUpdated(audio_track_info);
      });
}

void RemoteMediaPlayer::OnTimeUpdate(base::TimeDelta current_timestamp,
                                     base::TimeTicks current_time_ticks) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnTimeUpdate(current_timestamp, base::TimeTicks::Now());
  });
}

void RemoteMediaPlayer::OnActiveRegionChanged(
    const blink::WebRect& active_region) {
  listeners_.ForAllPtrs([&active_region](mojom::MediaPlayerListener* listener) {
    listener->OnActiveRegionChanged(gfx::Rect(active_region.x, active_region.y,
                                              active_region.width,
                                              active_region.height));
  });
}

void RemoteMediaPlayer::OnAudioFocusChanged() {
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnAudioFocusChanged();
  });
}

MediaPlayerProviderImpl::MediaPlayerProviderImpl() {}
MediaPlayerProviderImpl::~MediaPlayerProviderImpl() {}

void MediaPlayerProviderImpl::CreateMediaPlayer(
    mojom::MediaPlayerType media_player_type,
    const std::string& app_id,
    mojom::MediaPlayerRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<RemoteMediaPlayer>(media_player_type, app_id),
      std::move(request));
}

}  // namespace pal_media
