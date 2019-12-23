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

#include "content/browser/media/neva/media_state_manager_impl.h"

#include "base/memory/singleton.h"
#include "base/unguessable_token.h"
#include "content/browser/media/neva/media_state_policy_factory.h"
#include "content/public/browser/render_widget_host_view.h"

namespace content {

struct MediaStateManagerImpl::VideoWindowInfo {
  enum class State {
    kNone,
    kCreating,   // create is requested
    kCreated,    //
    kDestroying  // destroy is requested
  };
  VideoWindowInfo(const MediaPlayerId& id) : id_(id) {}
  ~VideoWindowInfo() = default;
  MediaPlayerId id_;
  base::UnguessableToken window_id_;
  std::string native_window_name_;
  bool activation_requested_ = false;
  State state_ = State::kNone;
};

MediaStateManager* MediaStateManager::GetInstance() {
  return MediaStateManagerImpl::GetInstance();
}

MediaStateManagerImpl* MediaStateManagerImpl::GetInstance() {
  return base::Singleton<MediaStateManagerImpl>::get();
}

MediaStateManagerImpl::MediaStateManagerImpl() {
  policy_.reset(MediaStatePolicyFactory::CreateMediaStatePolicy(this));
}

MediaStateManagerImpl::~MediaStateManagerImpl() {
  policy_.reset(nullptr);
}

void MediaStateManagerImpl::OnMediaActivated(RenderFrameHost* render_frame_host,
                                             int player_id) {
  policy_->OnMediaActivated(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaActivationRequested(
    RenderFrameHost* render_frame_host,
    int player_id) {
  MediaPlayerId player(render_frame_host, player_id);
  auto it = player_to_id_map_.find(player);
  if (it == player_to_id_map_.end()) {
    LOG(ERROR) << __func__
               << " failed to find video window id. request video window again";
    RequestVideoWindow(player, true);
    return;
  }
  VideoWindowInfo* info = FindVideoWindowInfo(it->second);
  if (!info || info->state_ == VideoWindowInfo::State::kNone ||
      info->state_ == VideoWindowInfo::State::kDestroying) {
    LOG(ERROR) << __func__
               << " failed to find window info. request video window again";
    RequestVideoWindow(player, true);
    return;
  }

  if (info->state_ == VideoWindowInfo::State::kCreating) {
    // Video window is requested but not yet created.
    info->activation_requested_ = true;
    return;
  }

  policy_->OnMediaActivationRequested(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaCreated(RenderFrameHost* render_frame_host,
                                           int player_id,
                                           bool will_use_media_resource) {
  RequestVideoWindow(std::make_pair(render_frame_host, player_id), false);

  policy_->OnMediaCreated(render_frame_host, player_id,
                          will_use_media_resource);
}

void MediaStateManagerImpl::OnMediaDestroyed(RenderFrameHost* render_frame_host,
                                             int player_id) {
  RemoveVideoWindow(std::make_pair(render_frame_host, player_id));
  policy_->OnMediaDestroyed(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaSuspended(RenderFrameHost* render_frame_host,
                                             int player_id) {
  policy_->OnMediaSuspended(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnRenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  policy_->OnRenderFrameDeleted(render_frame_host);
}

void MediaStateManagerImpl::OnWebContentsDestroyed(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    OnRenderFrameDeleted(rfh);
}

void MediaStateManagerImpl::ResumeAllMedia(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    rfh->SetSuppressed(false);
  policy_->OnMediaResumeRequested(web_contents);
}

void MediaStateManagerImpl::ResumeAllMedia(RenderFrameHost* render_frame_host) {
  render_frame_host->SetSuppressed(false);
  policy_->OnMediaResumeRequested(render_frame_host);
}

void MediaStateManagerImpl::SuspendAllMedia(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    rfh->SetSuppressed(true);
  policy_->OnMediaSuspendRequested(web_contents);
}

void MediaStateManagerImpl::SuspendAllMedia(
    RenderFrameHost* render_frame_host) {
  render_frame_host->SetSuppressed(true);
  policy_->OnMediaSuspendRequested(render_frame_host);
}

void MediaStateManagerImpl::PermitMediaActivation(
    RenderFrameHost* render_frame_host,
    int player_id) {
  render_frame_host->PermitMediaActivation(player_id);
}

void MediaStateManagerImpl::SuspendMedia(RenderFrameHost* render_frame_host,
                                         int player_id) {
  render_frame_host->SuspendMedia(player_id);
}

void MediaStateManagerImpl::OnVideoWindowCreated(
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__ << " unknown window_id=" << window_id;
    return;
  }

  info->window_id_ = window_id;
  info->native_window_name_ = vwch_.GetNativeLayerId(window_id);
  info->state_ = VideoWindowInfo::State::kCreated;

  RenderFrameHost* render_frame_host = info->id_.first;
  int player_id = info->id_.second;

  content::MediaLayerInfo layer_info;
  layer_info.media_layer_id_ = info->native_window_name_;
  layer_info.overlay_plane_token_ = info->window_id_;

  VLOG(1) << __func__ << " Send id(" << layer_info.media_layer_id_
          << ") with token(" << layer_info.overlay_plane_token_;

  render_frame_host->NotifyMediaLayerCreated(player_id, layer_info);

  // continue MediaActivation if requested
  if (info->activation_requested_)
    policy_->OnMediaActivationRequested(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnVideoWindowDestroyed(
    const base::UnguessableToken& window_id) {
  VLOG(1) << __func__ << " window_id=" << window_id;
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__ << " unknown window_id=" << window_id;
    return;
  }
  info->id_.first->NotifyMediaLayerWillDestroyed(info->id_.second);
  if (!id_to_info_map_.erase(window_id))
    LOG(ERROR) << __func__
               << " faild to erase id_to_info_map_ for window_id=" << window_id;
  if (!player_to_id_map_.erase(info->id_))
    LOG(ERROR) << __func__ << " faild to erase player_to_id_map_ for window_id="
               << window_id;
}

void MediaStateManagerImpl::OnVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__
               << " failed to find player info for id=" << window_id;
    return;
  }

  RenderFrameHost* host = info->id_.first;
  int player_id = info->id_.second;
  RenderWidgetHostView* view = host->GetView();
  if (!view) {
    LOG(ERROR) << __func__
               << " no RenderWidgetHostView for player_id=" << player_id
               << " window_id=" << window_id;
    return;
  }
  // TODO(neva): we may use view offset to get absolute position.
  gfx::Rect view_rect = view->GetViewBounds();
  VLOG(1) << __func__ << " view_rect=" << view_rect.ToString()
          << " video_rect=" << rect.ToString() << " window_id=" << window_id;
  host->NotifyMediaLayerGeometryChanged(player_id, rect);
}

void MediaStateManagerImpl::OnVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__
               << " failed to find player info for id=" << window_id;
    return;
  }

  RenderFrameHost* host = info->id_.first;
  int player_id = info->id_.second;
  host->NotifyMediaLayerVisibilityChanged(player_id, visibility);
}

void MediaStateManagerImpl::RequestVideoWindow(MediaPlayerId player,
                                               bool from_activation) {
  gfx::AcceleratedWidget owner = player.first->GetAcceleratedWidget();
  base::UnguessableToken window_id = vwch_.CreateVideoWindow(owner);
  VLOG(1) << __func__ << " owner=" << owner << " id=" << window_id;
  player_to_id_map_[player] = window_id;
  auto result = id_to_info_map_.emplace(
      window_id, std::make_unique<VideoWindowInfo>(player));
  VideoWindowInfo* info = result.first->second.get();
  info->state_ = VideoWindowInfo::State::kCreating;
  info->activation_requested_ = from_activation;
}

void MediaStateManagerImpl::RemoveVideoWindow(MediaPlayerId player) {
  auto it = player_to_id_map_.find(player);
  if (it == player_to_id_map_.end()) {
    LOG(ERROR) << __func__ << " failed to find window id for (" << player.first
               << "," << player.second << ")";
    return;
  }

  VideoWindowInfo* info = FindVideoWindowInfo(it->second);
  if (!info) {
    LOG(INFO) << __func__ << " window id" << it->second
              << " is already destroyed";
    return;
  }

  if (info->state_ == VideoWindowInfo::State::kDestroying) {
    LOG(INFO) << __func__ << " window id" << it->second << " is destroying";
    return;
  }

  VLOG(1) << __func__ << " id=" << it->second;
  info->state_ = VideoWindowInfo::State::kDestroying;
  vwch_.DestroyVideoWindow(it->second);
}

MediaStateManagerImpl::VideoWindowInfo*
MediaStateManagerImpl::FindVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto it = id_to_info_map_.find(window_id);
  if (it == id_to_info_map_.end())
    return nullptr;
  return it->second.get();
}

}  // namespace content
