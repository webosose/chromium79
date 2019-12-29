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

#include "neva/pal_media_service/webos/mediaplayer_ums.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "ui/gfx/geometry/rect_f.h"

namespace pal_media {

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   (media::BindToCurrentLoop(base::Bind(function, AsWeakPtr()))))

static mojom::MediaError convertToMediaError(media::PipelineStatus status) {
  switch (status) {
    case media::PipelineStatus::PIPELINE_OK:
      return mojom::MediaError::kMediaErrorNone;

    case media::PipelineStatus::PIPELINE_ERROR_NETWORK:
      return mojom::MediaError::kMediaErrorInvalidCode;

    case media::PipelineStatus::PIPELINE_ERROR_DECODE:
      return mojom::MediaError::kMediaErrorDecode;

    case media::PipelineStatus::PIPELINE_ERROR_DECRYPT:
    case media::PipelineStatus::PIPELINE_ERROR_ABORT:
    case media::PipelineStatus::PIPELINE_ERROR_INITIALIZATION_FAILED:
    case media::PipelineStatus::PIPELINE_ERROR_COULD_NOT_RENDER:
    case media::PipelineStatus::PIPELINE_ERROR_READ:
    case media::PipelineStatus::PIPELINE_ERROR_INVALID_STATE:
      return mojom::MediaError::kMediaErrorFormat;

    // Demuxer related errors.
    case media::PipelineStatus::DEMUXER_ERROR_COULD_NOT_OPEN:
    case media::PipelineStatus::DEMUXER_ERROR_COULD_NOT_PARSE:
    case media::PipelineStatus::DEMUXER_ERROR_NO_SUPPORTED_STREAMS:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // Decoder related errors.
    case media::PipelineStatus::DECODER_ERROR_NOT_SUPPORTED:
      return mojom::MediaError::kMediaErrorFormat;

    // resource is released by policy action
    case media::PipelineStatus::DECODER_ERROR_RESOURCE_IS_RELEASED:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // ChunkDemuxer related errors.
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_APPEND_FAILED:
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_EOS_STATUS_DECODE_ERROR:
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_EOS_STATUS_NETWORK_ERROR:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // Audio rendering errors.
    case media::PipelineStatus::AUDIO_RENDERER_ERROR:
      return mojom::MediaError::kMediaErrorInvalidCode;

    default:
      return mojom::MediaError::kMediaErrorInvalidCode;
  }
  return mojom::MediaError::kMediaErrorNone;
}

MediaPlayerUMS::MediaPlayerUMS()
    : paused_(true),
      playback_rate_(1.0f),
      is_suspended_(false),
      fullscreen_(false),
      active_video_region_changed_(false),
      is_video_offscreen_(false),
      main_task_runner_(
          base::CreateSingleThreadTaskRunner({base::ThreadPool()})),
      weak_factory_(this) {
  LOG(ERROR) << __func__;
  umedia_client_.reset(WebOSMediaClient::Create(main_task_runner_));
}

MediaPlayerUMS::~MediaPlayerUMS() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MediaPlayerUMS::Initialize(const bool is_video,
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

  umedia_client_->Load(
      is_video, current_time, false, app_id, url, mime_type, referrer,
      user_agent, cookies, media_option,
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnPlaybackStateChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnStreamEnded),
      BIND_TO_RENDER_LOOP(&MediaPlayerUMS::OnSeekDone),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnError),
      BIND_TO_RENDER_LOOP(&MediaPlayerUMS::OnBufferingState),
      BIND_TO_RENDER_LOOP(
          &MediaPlayerBaseImpl::OnDurationChange),  // not implemented
      BIND_TO_RENDER_LOOP(&MediaPlayerUMS::OnVideoSizeChange),
      BIND_TO_RENDER_LOOP(
          &MediaPlayerUMS::OnVideoDisplayWindowChange),  // not using listeners_
      BIND_TO_RENDER_LOOP(
          &MediaPlayerBaseImpl::OnAddAudioTrack),  // not implemented
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnAddVideoTrack),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::UpdateUMSInfo),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnAudioFocusChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::ActiveRegionChanged),
      BIND_TO_RENDER_LOOP(
          &MediaPlayerBaseImpl::OnWaitingForDecryptionKey),  // not
                                                             // implemented
      BIND_TO_RENDER_LOOP(
          &MediaPlayerBaseImpl::OnEncryptedMediaInitData));  // not
                                                             // implemented
}

void MediaPlayerUMS::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SetPlaybackRate(playback_rate_);
  paused_ = false;

  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(
                                            pal_media::kTimeUpdateInterval),
                             this, &MediaPlayerUMS::OnTimeUpdateTimerFired);
  }
}

void MediaPlayerUMS::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SetPlaybackRate(0.0f);
  time_update_timer_.Stop();
  paused_ = true;
  paused_time_ =
      base::TimeDelta::FromSecondsD(umedia_client_->GetCurrentTime());
}

void MediaPlayerUMS::Seek(const base::TimeDelta time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->Seek(time, BIND_TO_RENDER_LOOP(&MediaPlayerUMS::OnSeekDone));
}

void MediaPlayerUMS::SetVolume(double volume) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SetPlaybackVolume(volume);
}

void MediaPlayerUMS::SetPoster(const GURL& poster) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
}

void MediaPlayerUMS::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  if (!umedia_client_->IsSupportedBackwardTrickPlay() && rate < 0.0)
    return;

  playback_rate_ = rate;
  if (!paused_)
    umedia_client_->SetPlaybackRate(playback_rate_);
}

void MediaPlayerUMS::SetPreload(mojom::Preload preload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SetPreload(static_cast<WebOSMediaClient::Preload>(preload));
}

void MediaPlayerUMS::IsPreloadable(const std::string& content_media_option,
                                   IsPreloadableCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(umedia_client_->IsPreloadable(content_media_option));
}

void MediaPlayerUMS::HasVideo(HasVideoCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(umedia_client_->HasVideo());
}

void MediaPlayerUMS::HasAudio(HasAudioCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(umedia_client_->HasAudio());
}

void MediaPlayerUMS::SelectTrack(const mojom::MediaTrackType type,
                                 const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SelectTrack(type, id);
}

void MediaPlayerUMS::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  umedia_client_->SwitchToAutoLayout();
}

void MediaPlayerUMS::SetDisplayWindow(const gfx::Rect& outRect,
                                      const gfx::Rect& inRect,
                                      bool fullScreen,
                                      bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1) << "outRect:" << outRect.ToString()
           << " inRect:" << inRect.ToString();
  display_window_out_rect_ = outRect;
  display_window_in_rect_ = inRect;
  umedia_client_->SetDisplayWindow(outRect, inRect, fullScreen, forced);
}

void MediaPlayerUMS::UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->UsesIntrinsicSize());
}

void MediaPlayerUMS::MediaId(MediaIdCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(umedia_client_->MediaId());
}

void MediaPlayerUMS::HasAudioFocus(HasAudioFocusCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  bool result = (umedia_client_ ? umedia_client_->Focus() : false);
  std::move(callback).Run(result);
}

void MediaPlayerUMS::SetAudioFocus(bool focus) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  if (umedia_client_ && focus && !umedia_client_->Focus()) {
    umedia_client_->SetFocus();
  }
}

void MediaPlayerUMS::HasVisibility(HasVisibilityCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(umedia_client_->Visibility());
}

void MediaPlayerUMS::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << " visibility=" << visibility;
  if (!is_video_offscreen_)
    umedia_client_->SetVisibility(visibility);
}

void MediaPlayerUMS::Suspend(mojom::SuspendReason reason) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  if (is_suspended_)
    return;

  is_suspended_ = true;
  umedia_client_->Suspend(reason);
}

void MediaPlayerUMS::Resume() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  if (!is_suspended_)
    return;

  is_suspended_ = false;
  umedia_client_->Resume();
}

void MediaPlayerUMS::RequireMediaResource(
    RequireMediaResourceCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(true);
}

void MediaPlayerUMS::IsRecoverableOnResume(
    IsRecoverableOnResumeCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->IsRecoverableOnResume());
}

void MediaPlayerUMS::SetDisableAudio(bool disable) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  umedia_client_->SetDisableAudio(disable);
}

void MediaPlayerUMS::OnStreamEnded() {
  DVLOG(1);
  time_update_timer_.Stop();
  MediaPlayerBaseImpl::OnStreamEnded();
}

void MediaPlayerUMS::ActiveRegionChanged(const gfx::Rect& active_region) {
  DVLOG(1) << gfx::Rect(active_region).ToString();
  if (active_video_region_ != active_region) {
    active_video_region_ = active_region;
    active_video_region_changed_ = true;
  }
  MediaPlayerBaseImpl::ActiveRegionChanged(active_video_region_);
}

void MediaPlayerUMS::OnSeekDone(media::PipelineStatus status) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);
  MediaPlayerBaseImpl::OnSeekDone(status, GetCurrentTime());
}

void MediaPlayerUMS::OnBufferingState(
    WebOSMediaClient::BufferingState buffering_state) {
  DVLOG(2) << " state:" << buffering_state;
  MediaPlayerBaseImpl::OnBufferingState(buffering_state,
                                        umedia_client_->GetDuration(),
                                        umedia_client_->GetNaturalVideoSize());
}

void MediaPlayerUMS::OnVideoSizeChange() {
  DVLOG(2);
  MediaPlayerBaseImpl::OnVideoSizeChange(umedia_client_->GetNaturalVideoSize());
}

void MediaPlayerUMS::OnVideoDisplayWindowChange() {
  DVLOG(1);
  umedia_client_->SetDisplayWindow(display_window_out_rect_,
                                   display_window_in_rect_, fullscreen_, true);
}

base::TimeDelta MediaPlayerUMS::GetCurrentTime() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);
  return base::TimeDelta::FromSecondsD(umedia_client_->GetCurrentTime());
}

void MediaPlayerUMS::OnTimeUpdateTimerFired() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);
  MediaPlayerBaseImpl::OnTimeUpdateTimerFired(GetCurrentTime());
}

}  // namespace pal_media
