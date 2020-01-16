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

#include "media/blink/neva/mediaplayer_mojo.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "content/browser/system_connector_impl.h"
#include "content/child/child_thread_impl.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/renderer/render_view_impl.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/gfx/geometry/rect_f.h"

namespace media {

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

static MediaPlayerNeva::MediaError ConvertPalMediaError(
    pal_media::mojom::MediaError error_type) {
  switch (error_type) {
    case pal_media::mojom::MediaError::kMediaErrorNone:
      return MediaPlayerNeva::MEDIA_ERROR_NONE;

    case pal_media::mojom::MediaError::kMediaErrorFormat:
      return MediaPlayerNeva::MEDIA_ERROR_FORMAT;

    case pal_media::mojom::MediaError::kMediaErrorDecode:
      return MediaPlayerNeva::MEDIA_ERROR_DECODE;

    case pal_media::mojom::MediaError::kMediaNotValidForProgressivePlayback:
      return MediaPlayerNeva::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK;

    case pal_media::mojom::MediaError::kMediaErrorInvalidCode:
    default:
      return MediaPlayerNeva::MEDIA_ERROR_INVALID_CODE;
  }
  return MediaPlayerNeva::MEDIA_ERROR_NONE;
}

static blink::WebMediaPlayer::MediaEventType ConvertPalMediaEventType(
    pal_media::mojom::MediaEventType event_type) {
  switch (event_type) {
    case pal_media::mojom::MediaEventType::kMediaEventNone:
      return blink::WebMediaPlayer::kMediaEventNone;
    case pal_media::mojom::MediaEventType::kMediaEventUpdateUMSMediaInfo:
      return blink::WebMediaPlayer::kMediaEventUpdateUMSMediaInfo;
    case pal_media::mojom::MediaEventType::kMediaEventBroadcastErrorMsg:
      return blink::WebMediaPlayer::kMediaEventBroadcastErrorMsg;
    case pal_media::mojom::MediaEventType::kMediaEventDvrErrorMsg:
      return blink::WebMediaPlayer::kMediaEventDvrErrorMsg;
    case pal_media::mojom::MediaEventType::kMediaEventUpdateCameraState:
      return blink::WebMediaPlayer::kMediaEventUpdateCameraState;
    case pal_media::mojom::MediaEventType::kMediaEventPipelineStarted:
      return blink::WebMediaPlayer::kMediaEventPipelineStarted;
  }
}

MediaPlayerMojo::MediaPlayerMojo(
    MediaPlayerNevaClient* client,
    pal_media::mojom::MediaPlayerType media_player_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner)
    : client_(client),
      main_task_runner_(main_task_runner),
      listener_binding_(this) {
  DVLOG(1) << __func__;

  service_manager::Connector* connector = content::ChildThreadImpl::current()
                                              ->GetServiceManagerConnection()
                                              ->GetConnector();
  if (!connector)
    return;

  mojo::Remote<pal_media::mojom::MediaPlayerProvider> provider;
  connector->Connect(pal_media::mojom::kServiceName,
                     provider.BindNewPipeAndPassReceiver());

  provider->GetMediaPlayer(media_player_type,
                           mojo::MakeRequest(&media_player_));
  media_player_->Subscribe(base::BindOnce(&MediaPlayerMojo::OnSubscribeRespond,
                                          base::Unretained(this)));
}

MediaPlayerMojo::~MediaPlayerMojo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MediaPlayerMojo::Initialize(const bool is_video,
                                 const double current_time,
                                 const std::string& app_id,
                                 const std::string& url,
                                 const std::string& mime_type,
                                 const std::string& referrer,
                                 const std::string& user_agent,
                                 const std::string& cookies,
                                 const std::string& media_option,
                                 const std::string& custom_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << " app_id: " << app_id << " / url: " << url
           << " / media_option: " << media_option;

  if (media_player_)
    media_player_->Initialize(is_video, current_time, app_id, url, mime_type,
                              referrer, user_agent, cookies, media_option,
                              custom_option);
}

void MediaPlayerMojo::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Start();
}

void MediaPlayerMojo::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Pause();
}

void MediaPlayerMojo::Seek(const base::TimeDelta& time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Seek(time);
}

void MediaPlayerMojo::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetVolume(volume);
}

void MediaPlayerMojo::SetPoster(const GURL& poster) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
}

void MediaPlayerMojo::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetRate(rate);
}

void MediaPlayerMojo::SetPreload(MediaPlayerNeva::Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return;
  switch (preload) {
    case MediaPlayerNeva::PreloadNone:
      media_player_->SetPreload(pal_media::mojom::Preload::kPreloadNone);
      break;
    case MediaPlayerNeva::PreloadMetaData:
      media_player_->SetPreload(pal_media::mojom::Preload::kPreloadMetaData);
      break;
    case MediaPlayerNeva::PreloadAuto:
      media_player_->SetPreload(pal_media::mojom::Preload::kPreloadAuto);
      break;
    default:
      break;
  }
}

bool MediaPlayerMojo::IsPreloadable(const std::string& content_media_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ &&
      media_player_->IsPreloadable(content_media_option, &result))
    return result;
  return false;
}

bool MediaPlayerMojo::HasVideo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVideo(&result))
    return result;
  return false;
}

bool MediaPlayerMojo::HasAudio() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudio(&result))
    return result;
  return false;
}

bool MediaPlayerMojo::SelectTrack(const MediaTrackType type,
                                  const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return false;
  media_player_->SelectTrack(type, id);
  return true;
}

void MediaPlayerMojo::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SwitchToAutoLayout();
}

void MediaPlayerMojo::SetDisplayWindow(const gfx::Rect& out_rect,
                                       const gfx::Rect& in_rect,
                                       bool fullscreen,
                                       bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << "out_rect:" << out_rect.ToString()
           << " in_rect:" << in_rect.ToString();
  if (media_player_)
    media_player_->SetDisplayWindow(out_rect, in_rect, fullscreen, forced);
}

bool MediaPlayerMojo::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->UsesIntrinsicSize(&result))
    return result;
  return false;
}

std::string MediaPlayerMojo::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  std::string result = "";
  if (media_player_)
    media_player_->MediaId(&result);
  return result;
}

bool MediaPlayerMojo::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudioFocus(&result))
    return result;
  return false;
}

void MediaPlayerMojo::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetAudioFocus(focus);
}

bool MediaPlayerMojo::HasVisibility() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVisibility(&result))
    return result;
  return false;
}

void MediaPlayerMojo::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << " visibility=" << visibility;
  if (media_player_)
    media_player_->SetVisibility(visibility);
}

void MediaPlayerMojo::Suspend(SuspendReason reason) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return;
  switch (reason) {
    case SuspendReason::BACKGROUNDED:
      media_player_->Suspend(pal_media::mojom::SuspendReason::kBackground);
      break;
    case SuspendReason::SUSPENDED_BY_POLICY:
      media_player_->Suspend(
          pal_media::mojom::SuspendReason::kSuspendedByPolicy);
      break;
    default:
      break;
  }
}

void MediaPlayerMojo::Resume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Resume();
}

bool MediaPlayerMojo::RequireMediaResource() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->RequireMediaResource(&result))
    return result;
  return false;
}

bool MediaPlayerMojo::IsRecoverableOnResume() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->IsRecoverableOnResume(&result))
    return result;
  return false;
}

void MediaPlayerMojo::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (media_player_)
    media_player_->SetDisableAudio(disable);
}

void MediaPlayerMojo::OnMediaPlayerPlay() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPlay();
}

void MediaPlayerMojo::OnMediaPlayerPause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPause();
}

void MediaPlayerMojo::OnPlaybackComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnPlaybackComplete();
}

void MediaPlayerMojo::OnMediaError(pal_media::mojom::MediaError error_type) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaError(ConvertPalMediaError(error_type));
}

void MediaPlayerMojo::OnSeekComplete(base::TimeDelta current_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnSeekComplete(current_time);
}

void MediaPlayerMojo::OnMediaMetadataChanged(base::TimeDelta duration,
                                             uint32_t width,
                                             uint32_t height,
                                             bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaMetadataChanged(duration, width, height, success);
}

void MediaPlayerMojo::OnLoadComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnLoadComplete();
}

void MediaPlayerMojo::OnVideoSizeChanged(uint32_t width, uint32_t height) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnVideoSizeChanged(width, height);
}

void MediaPlayerMojo::OnCustomMessage(
    const pal_media::mojom::MediaEventType media_event_type,
    const std::string& detail) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnCustomMessage(ConvertPalMediaEventType(media_event_type), detail);
}

void MediaPlayerMojo::OnTimeUpdate(base::TimeDelta current_timestamp,
                                   base::TimeTicks current_time_ticks) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnTimeUpdate(current_timestamp, current_time_ticks);
}

void MediaPlayerMojo::OnAudioTracksUpdated(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioTracksUpdated(audio_track_info);
}

void MediaPlayerMojo::OnAudioFocusChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioFocusChanged();
}

void MediaPlayerMojo::OnActiveRegionChanged(const gfx::Rect& active_region) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnActiveRegionChanged(active_region);
}

void MediaPlayerMojo::OnSubscribeRespond(
    pal_media::mojom::MediaPlayerListenerAssociatedRequest request) {
  listener_binding_.Bind(std::move(request));
}

}  // namespace media
