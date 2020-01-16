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

#include "neva/pal_media_service/webos/mediaplayer_camera.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace pal_media {

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

MediaPlayerCamera::MediaPlayerCamera()
    : playback_rate_(1.0f),
      is_video_offscreen_(false),
      fullscreen_(false),
      main_task_runner_(
          base::CreateSingleThreadTaskRunner({base::ThreadPool()})),
      weak_factory_(this) {
  LOG(INFO) << __func__;
  umedia_client_.reset(WebOSMediaClient::Create(main_task_runner_));
}

MediaPlayerCamera::~MediaPlayerCamera() {
  LOG(INFO) << __func__;
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MediaPlayerCamera::Initialize(const bool is_video,
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
  DVLOG(2) << " app_id: " << app_id.c_str() << " url : " << url.c_str()
           << " custom_option - "
           << (custom_option.empty() ? "{}" : custom_option.c_str());

  app_id_ = app_id;
  url::Parsed parsed;
  url::ParseStandardURL(url.c_str(), url.length(), &parsed);
  url_ = GURL(url, parsed, true);
  mime_type_ = mime_type;

  umedia_client_->Load(
      is_video, current_time, false, app_id, url, mime_type, referrer,
      user_agent, cookies, custom_option,
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnPlaybackStateChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnStreamEnded),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnSeekDone),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnError),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnBufferingState),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnDurationChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnVideoSizeChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnVideoDisplayWindowChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnAddAudioTrack),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnAddVideoTrack),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnUpdateUMSInfo),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnAudioFocusChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnActiveRegionChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnWaitingForDecryptionKey),
      BIND_TO_RENDER_LOOP(&MediaPlayerBaseImpl::OnEncryptedMediaInitData));
}

void MediaPlayerCamera::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);

  umedia_client_->SetPlaybackRate(playback_rate_);
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(
                                            pal_media::kTimeUpdateInterval),
                             this, &MediaPlayerCamera::OnTimeUpdateTimerFired);
  }
}

void MediaPlayerCamera::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);

  time_update_timer_.Stop();
}

void MediaPlayerCamera::IsPreloadable(const std::string& content_media_option,
                                      IsPreloadableCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(1);
  std::move(callback).Run(false);
}

void MediaPlayerCamera::HasVideo(HasVideoCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->HasVideo());
}

void MediaPlayerCamera::HasAudio(HasAudioCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->HasAudio());
}

void MediaPlayerCamera::SelectTrack(const media::MediaTrackType type,
                                    const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  NOTIMPLEMENTED();
}

void MediaPlayerCamera::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  umedia_client_->SwitchToAutoLayout();
}

void MediaPlayerCamera::SetDisplayWindow(const gfx::Rect& out_rect,
                                         const gfx::Rect& in_rect,
                                         bool full_screen,
                                         bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  display_window_out_rect_ = out_rect;
  display_window_in_rect_ = in_rect;
  umedia_client_->SetDisplayWindow(out_rect, in_rect, full_screen, forced);
}

void MediaPlayerCamera::UsesIntrinsicSize(UsesIntrinsicSizeCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->UsesIntrinsicSize());
}

void MediaPlayerCamera::MediaId(MediaIdCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->MediaId());
}

void MediaPlayerCamera::IsRecoverableOnResume(
    IsRecoverableOnResumeCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(true);
}
void MediaPlayerCamera::HasAudioFocus(HasAudioFocusCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(false);
}

void MediaPlayerCamera::HasVisibility(HasVisibilityCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(umedia_client_->Visibility());
}

void MediaPlayerCamera::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!is_video_offscreen_)
    umedia_client_->SetVisibility(visibility);
}

void MediaPlayerCamera::RequireMediaResource(
    RequireMediaResourceCallback callback) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(true);
}

void MediaPlayerCamera::OnPlaybackStateChanged(bool playing) {
  DVLOG(1);

  Json::Value root;
  root["mediaId"] = umedia_client_->MediaId().c_str();
  root["infoType"] = "cameraState";

  if (playing) {
    root["cameraState"] = "playing";
  } else {
    root["cameraState"] = "paused";
  }

  MediaPlayerBaseImpl::OnPlaybackStateChanged(playing);

  Json::FastWriter writer;

  MediaPlayerBaseImpl::OnCustomMessage(
      pal_media::mojom::MediaEventType::kMediaEventUpdateCameraState,
      writer.write(root));
}

void MediaPlayerCamera::OnStreamEnded() {
  DVLOG(1);
  time_update_timer_.Stop();
}

void MediaPlayerCamera::OnBufferingState(
    WebOSMediaClient::BufferingState buffering_state) {
  switch (buffering_state) {
    case WebOSMediaClient::kHaveMetadata:
      umedia_client_->SetPlaybackRate(playback_rate_);
      break;
    default:
      break;
  }
  MediaPlayerBaseImpl::OnBufferingState(buffering_state,
                                        umedia_client_->GetDuration(),
                                        umedia_client_->GetNaturalVideoSize());
}

void MediaPlayerCamera::OnVideoSizeChange() {
  DVLOG(1);
  MediaPlayerBaseImpl::OnVideoSizeChange(umedia_client_->GetNaturalVideoSize());
}

void MediaPlayerCamera::OnVideoDisplayWindowChange() {
  DVLOG(1);
  umedia_client_->SetDisplayWindow(display_window_out_rect_,
                                   display_window_in_rect_, fullscreen_, true);
}

void MediaPlayerCamera::OnUpdateUMSInfo(const std::string& detail) {
  DVLOG(1);

  if (detail.empty())
    return;

  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(detail, root)) {
    LOG(ERROR) << " Json::Reader::parse (" << detail << ") - Failed!!!";
    return;
  }

  if (root.isMember("cameraId"))
    camera_id_ = root["cameraId"].asString();

  MediaPlayerBaseImpl::OnCustomMessage(
      pal_media::mojom::MediaEventType::kMediaEventUpdateCameraState, detail);
}

void MediaPlayerCamera::OnActiveRegionChanged(const gfx::Rect& active_region) {
  DVLOG(1) << gfx::Rect(active_region).ToString();

  if (active_video_region_ != active_region)
    active_video_region_ = active_region;

  MediaPlayerBaseImpl::ActiveRegionChanged(active_video_region_);
}

base::TimeDelta MediaPlayerCamera::GetCurrentTime() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);
  return base::TimeDelta::FromSecondsD(umedia_client_->GetCurrentTime());
}

void MediaPlayerCamera::OnTimeUpdateTimerFired() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DVLOG(2);
  MediaPlayerBaseImpl::OnTimeUpdateTimerFired(GetCurrentTime());
}

}  // namespace pal_media
