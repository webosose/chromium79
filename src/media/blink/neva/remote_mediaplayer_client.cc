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

#include "media/blink/neva/remote_mediaplayer_client.h"

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

RemoteMediaPlayerClient::RemoteMediaPlayerClient(
    MediaPlayerNevaClient* client,
    pal_media::mojom::MediaPlayerType media_player_type,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const std::string& app_id)
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

  provider->CreateMediaPlayer(media_player_type, app_id,
                              mojo::MakeRequest(&media_player_));
  media_player_->Subscribe(base::BindOnce(
      &RemoteMediaPlayerClient::OnSubscribeRespond, base::Unretained(this)));
}

RemoteMediaPlayerClient::~RemoteMediaPlayerClient() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void RemoteMediaPlayerClient::Initialize(const bool is_video,
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

void RemoteMediaPlayerClient::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Start();
}

void RemoteMediaPlayerClient::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Pause();
}

void RemoteMediaPlayerClient::Seek(const base::TimeDelta& time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Seek(time);
}

void RemoteMediaPlayerClient::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetVolume(volume);
}

void RemoteMediaPlayerClient::SetPoster(const GURL& poster) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
}

void RemoteMediaPlayerClient::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetRate(rate);
}

void RemoteMediaPlayerClient::SetPreload(MediaPlayerNeva::Preload preload) {
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

bool RemoteMediaPlayerClient::IsPreloadable(
    const std::string& content_media_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ &&
      media_player_->IsPreloadable(content_media_option, &result))
    return result;
  return false;
}

bool RemoteMediaPlayerClient::HasVideo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVideo(&result))
    return result;
  return false;
}

bool RemoteMediaPlayerClient::HasAudio() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudio(&result))
    return result;
  return false;
}

bool RemoteMediaPlayerClient::SelectTrack(const MediaTrackType type,
                                          const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (!media_player_)
    return false;
  media_player_->SelectTrack(type, id);
  return true;
}

void RemoteMediaPlayerClient::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SwitchToAutoLayout();
}

void RemoteMediaPlayerClient::SetDisplayWindow(const gfx::Rect& out_rect,
                                               const gfx::Rect& in_rect,
                                               bool fullscreen,
                                               bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << "out_rect:" << out_rect.ToString()
           << " in_rect:" << in_rect.ToString();
  if (media_player_)
    media_player_->SetDisplayWindow(out_rect, in_rect, fullscreen, forced);
}

bool RemoteMediaPlayerClient::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->UsesIntrinsicSize(&result))
    return result;
  return false;
}

std::string RemoteMediaPlayerClient::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  std::string result = "";
  if (media_player_)
    media_player_->MediaId(&result);
  return result;
}

bool RemoteMediaPlayerClient::HasAudioFocus() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasAudioFocus(&result))
    return result;
  return false;
}

void RemoteMediaPlayerClient::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->SetAudioFocus(focus);
}

bool RemoteMediaPlayerClient::HasVisibility() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  bool result;
  if (media_player_ && media_player_->HasVisibility(&result))
    return result;
  return false;
}

void RemoteMediaPlayerClient::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__ << " visibility=" << visibility;
  if (media_player_)
    media_player_->SetVisibility(visibility);
}

void RemoteMediaPlayerClient::Suspend(SuspendReason reason) {
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

void RemoteMediaPlayerClient::Resume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << __func__;
  if (media_player_)
    media_player_->Resume();
}

bool RemoteMediaPlayerClient::RequireMediaResource() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->RequireMediaResource(&result))
    return result;
  return false;
}

bool RemoteMediaPlayerClient::IsRecoverableOnResume() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  bool result;
  if (media_player_ && media_player_->IsRecoverableOnResume(&result))
    return result;
  return false;
}

media::Ranges<base::TimeDelta> RemoteMediaPlayerClient::GetBufferedTimeRanges()
    const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::vector<pal_media::mojom::TimeDeltaPairPtr> vector_ranges;
  media::Ranges<base::TimeDelta> ranges;
  if (media_player_) {
    media_player_->GetBufferedTimeRanges(&vector_ranges);
    for (const auto& r : vector_ranges) {
      ranges.Add(r->start, r->end);
    }
  }
  return ranges;
}

void RemoteMediaPlayerClient::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (media_player_)
    media_player_->SetDisableAudio(disable);
}

void RemoteMediaPlayerClient::SetMediaLayerId(
    const std::string& media_layer_id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (media_player_)
    media_player_->SetMediaLayerId(media_layer_id);
}

void RemoteMediaPlayerClient::OnMediaPlayerPlay() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPlay();
}

void RemoteMediaPlayerClient::OnMediaPlayerPause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaPlayerPause();
}

void RemoteMediaPlayerClient::OnPlaybackComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnPlaybackComplete();
}

void RemoteMediaPlayerClient::OnMediaError(int error) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaError(error);
}

void RemoteMediaPlayerClient::OnSeekComplete(base::TimeDelta current_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnSeekComplete(current_time);
}

void RemoteMediaPlayerClient::OnMediaMetadataChanged(base::TimeDelta duration,
                                                     uint32_t width,
                                                     uint32_t height,
                                                     bool success) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnMediaMetadataChanged(duration, width, height, success);
}

void RemoteMediaPlayerClient::OnLoadComplete() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnLoadComplete();
}

void RemoteMediaPlayerClient::OnVideoSizeChanged(uint32_t width,
                                                 uint32_t height) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnVideoSizeChanged(width, height);
}

void RemoteMediaPlayerClient::OnCustomMessage(
    const pal_media::mojom::MediaEventType media_event_type,
    const std::string& detail) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnCustomMessage(ConvertPalMediaEventType(media_event_type), detail);
}

void RemoteMediaPlayerClient::OnBufferingUpdate(int percentage) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnBufferingUpdate(percentage);
}

void RemoteMediaPlayerClient::OnTimeUpdate(base::TimeDelta current_timestamp,
                                           base::TimeTicks current_time_ticks) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnTimeUpdate(current_timestamp, current_time_ticks);
}

void RemoteMediaPlayerClient::OnAudioTracksUpdated(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioTracksUpdated(audio_track_info);
}

void RemoteMediaPlayerClient::OnAudioFocusChanged() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnAudioFocusChanged();
}

void RemoteMediaPlayerClient::OnActiveRegionChanged(
    const gfx::Rect& active_region) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  client_->OnActiveRegionChanged(active_region);
}

void RemoteMediaPlayerClient::OnSubscribeRespond(
    pal_media::mojom::MediaPlayerListenerAssociatedRequest request) {
  listener_binding_.Bind(std::move(request));
}

bool RemoteMediaPlayerClient::Send(const std::string& message) const {
  DVLOG(1) << __func__ << " , message: " << message;
  bool result;
  if (media_player_ && media_player_->Send(message, &result))
    return result;
  return false;
}

}  // namespace media
