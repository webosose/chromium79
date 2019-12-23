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

#include "ozone/media/video_window_controller_host_impl.h"

#include "base/bind.h"
#include "ozone/platform/messages.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"

namespace ui {

struct VideoWindowControllerHostImpl::VideoWindowInfo {
  enum class State { kNone, kCreating, kCreated, kDestroying, kDestroyed };

  explicit VideoWindowInfo(gfx::AcceleratedWidget w,
                           const base::UnguessableToken& id,
                           VideoWindowControllerHost::Client* client,
                           const std::string& native_id)
      : owner_(w), id_(id), client_(client), native_id_(native_id) {}
  ~VideoWindowInfo() = default;
  gfx::AcceleratedWidget owner_;
  base::UnguessableToken id_;
  VideoWindowControllerHost::Client* client_;
  std::string native_id_;
  State state_ = State::kNone;
};

VideoWindowControllerHostImpl::VideoWindowControllerHostImpl(
    OzoneGpuPlatformSupportHost* proxy)
    : proxy_(proxy) {
  proxy_->RegisterHandler(this);
}

VideoWindowControllerHostImpl::~VideoWindowControllerHostImpl() = default;

void VideoWindowControllerHostImpl::RegisterClient(Client* client) {
  if (clients_.find(client) != clients_.end()) {
    LOG(ERROR) << __func__ << " " << client << " is already registered";
    return;
  }
  clients_.insert(client);
}

void VideoWindowControllerHostImpl::UnRegisterClient(Client* client) {
  if (clients_.find(client) == clients_.end()) {
    LOG(ERROR) << __func__ << " " << client << " is not registered";
    return;
  }

  for (auto it = video_windows_.cbegin(); it != video_windows_.cend();) {
    if (it->second->client_ == client)
      video_windows_.erase(it++);
    else
      it++;
  }
  clients_.erase(client);
}

base::UnguessableToken VideoWindowControllerHostImpl::CreateVideoWindow(
    Client* client,
    gfx::AcceleratedWidget owner) {
  base::UnguessableToken window_id = base::UnguessableToken::Create();
  video_windows_.emplace(window_id, std::make_unique<VideoWindowInfo>(
                                        owner, window_id, client, ""));
  proxy_->Send(new WaylandDisplay_CreateVideoWindow(owner, window_id));
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (info)
    info->state_ = VideoWindowInfo::State::kCreating;
  return window_id;
}

void VideoWindowControllerHostImpl::DestroyVideoWindow(
    Client* client,
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__
               << " failed to find window info for window_id=" << window_id;
    return;
  }

  if (info->state_ == VideoWindowInfo::State::kDestroying) {
    LOG(ERROR) << __func__ << " window id" << window_id
               << " is already destroying";
    return;
  }

  info->state_ = VideoWindowInfo::State::kDestroying;
  proxy_->Send(new WaylandDisplay_DestroyVideoWindow(info->owner_, window_id));
}

std::string VideoWindowControllerHostImpl::GetNativeLayerId(
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__ << " failed to find window_id=" << window_id;
    return "";
  }
  return info->native_id_;
}

void VideoWindowControllerHostImpl::OnMessageReceived(
    const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(VideoWindowControllerHostImpl, message)
    IPC_MESSAGE_HANDLER(WaylandDisplay_VideoWindowCreated, OnVideoWindowCreated)
    IPC_MESSAGE_HANDLER(WaylandDisplay_VideoWindowGeometryChanged,
                        OnVideoWindowGeometryChanged)
    IPC_MESSAGE_HANDLER(WaylandDisplay_VideoWindowVisibilityChanged,
                        OnVideoWindowVisibilityChanged)
  IPC_END_MESSAGE_MAP()
}

void VideoWindowControllerHostImpl::OnVideoWindowCreated(
    unsigned w,
    const base::UnguessableToken& window_id,
    const std::string& native_id) {
  LOG(INFO) << __func__ << " w=" << w << " window_id=" << window_id;
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info || !info->client_) {
    LOG(ERROR) << __func__
               << " failed to find client for window_id=" << window_id;
    return;
  }
  if (info->state_ == VideoWindowInfo::State::kCreating)
    info->state_ = VideoWindowInfo::State::kCreated;
  else
    LOG(ERROR) << __func__ << " window id" << window_id
               << " recevied kCreated on state(" << (int)info->state_ << ")";
  info->native_id_ = native_id;
  info->client_->OnVideoWindowCreated(window_id);
}

void VideoWindowControllerHostImpl::OnVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  LOG(INFO) << __func__ << " window_id=" << window_id
            << " rect=" << rect.ToString();
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info || !info->client_) {
    LOG(ERROR) << __func__
               << " failed to find client for window_id=" << window_id;
    return;
  }
  info->client_->OnVideoWindowGeometryChanged(window_id, rect);
}

void VideoWindowControllerHostImpl::OnVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  LOG(INFO) << __func__ << " window_id=" << window_id
            << " visibility=" << visibility;
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info || !info->client_) {
    LOG(ERROR) << __func__
               << " failed to find client for window_id=" << window_id;
    return;
  }
  info->client_->OnVideoWindowVisibilityChanged(window_id, visibility);
}

void VideoWindowControllerHostImpl::OnVideoWindowDestroyed(
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info || !info->client_) {
    LOG(ERROR) << __func__
               << " failed to find client for window_id=" << window_id;
    return;
  }
  info->state_ = VideoWindowInfo::State::kDestroyed;
  info->client_->OnVideoWindowDestroyed(window_id);
}

VideoWindowControllerHostImpl::VideoWindowInfo*
VideoWindowControllerHostImpl::FindVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto it = video_windows_.find(window_id);
  if (it == video_windows_.end())
    return nullptr;
  return it->second.get();
}

}  // namespace ui
