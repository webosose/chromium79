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

#include "ozone/media/video_window_controller_impl.h"

#include "base/bind.h"
#include "ozone/platform/messages.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ozone/media/video_window_provider.h"

namespace ui {
VideoWindowControllerImpl::VideoWindowInfo::VideoWindowInfo(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& id,
    base::Optional<bool> visibility)
    : owner_widget_(w), id_(id), visibility_(visibility) {}

VideoWindowControllerImpl::VideoWindowInfo::VideoWindowInfo(
    const VideoWindowInfo&) = default;

VideoWindowControllerImpl::VideoWindowInfo::~VideoWindowInfo() = default;

VideoWindowControllerImpl::VideoWindowControllerImpl(
    VideoWindowSupport* support)
    : support_(support), provider_(VideoWindowProvider::Create(support)) {}

VideoWindowControllerImpl::~VideoWindowControllerImpl() = default;

void VideoWindowControllerImpl::CreateVideoWindow(
    unsigned w,
    const base::UnguessableToken& window_id) {
  InsertEmptyWindow(w, window_id);
  provider_->CreateNativeVideoWindow(
      w, window_id,
      base::BindRepeating(&VideoWindowControllerImpl::OnWindowEvent,
                          base::Unretained(this)));
}

void VideoWindowControllerImpl::InsertEmptyWindow(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& window_id) {
  id_to_widget_map_[window_id] = w;
  auto& wl = video_windows_[w];
  wl.emplace_back(w, window_id, base::nullopt);
}

VideoWindowControllerImpl::VideoWindowInfo*
VideoWindowControllerImpl::FindVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto widget_it = id_to_widget_map_.find(window_id);
  if (widget_it == id_to_widget_map_.end())
    return nullptr;
  auto wl_it = video_windows_.find(widget_it->second);
  if (wl_it == video_windows_.end())
    return nullptr;
  for (auto& window : wl_it->second)
    if (window.id_ == window_id)
      return &window;
  return nullptr;
}

void VideoWindowControllerImpl::RemoveVideoWindowInfo(
    const base::UnguessableToken& window_id) {
  auto widget_it = id_to_widget_map_.find(window_id);
  if (widget_it != id_to_widget_map_.end())
    return;
  gfx::AcceleratedWidget w = widget_it->second;
  id_to_widget_map_.erase(window_id);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end())
    return;

  for (auto it = wl_it->second.cbegin(); it != wl_it->second.cend(); it++) {
    if (it->id_ == window_id) {
      wl_it->second.erase(it);
      break;
    }
  }
}

void VideoWindowControllerImpl::DestroyVideoWindow(
    unsigned w,
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* info = FindVideoWindowInfo(window_id);
  if (!info) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  provider_->DestroyNativeVideoWindow(window_id);
}

void VideoWindowControllerImpl::NotifyVideoWindowGeometryChanged(
    const gpu::SurfaceHandle h,
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  VLOG(2) << __func__ << " window_id=" << window_id
          << " rect=" << rect.ToString();

  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto it = hidden_candidate_.find(w);
  if (it != hidden_candidate_.end()) {
    auto& hidden_candidate = it->second;
    hidden_candidate.erase(window_id);
  }

  SetVideoWindowVisibility(window_id, true);
  provider_->NativeVideoWindowGeometryChanged(window_id, rect);
}

void VideoWindowControllerImpl::SetVideoWindowVisibility(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VideoWindowInfo* w = FindVideoWindowInfo(window_id);
  if (!w) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }

  bool visibility_changed = false;
  if (w->visibility_.has_value() && w->visibility_.value() != visibility)
    visibility_changed = true;

  w->visibility_ = visibility;

  if (visibility_changed)
    provider_->NativeVideoWindowVisibilityChanged(window_id, visibility);
}

void VideoWindowControllerImpl::OnWindowEvent(
    const base::UnguessableToken& window_id,
    VideoWindowProvider::Event event) {
  switch (event) {
    case VideoWindowProvider::Event::kCreated:
      OnVideoWindowCreated(window_id);
      break;
    case VideoWindowProvider::Event::kDestroyed:
      OnVideoWindowDestroyed(window_id);
      break;
    default:
      break;
  }
}

void VideoWindowControllerImpl::OnVideoWindowCreated(
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* w = FindVideoWindowInfo(window_id);
  if (!w) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }

  std::string native_id = provider_->GetNativeVideoWindowName(window_id);
  support_->SendVideoWindowMessage(new WaylandDisplay_VideoWindowCreated(
      w->owner_widget_, window_id, native_id));
}

void VideoWindowControllerImpl::OnVideoWindowDestroyed(
    const base::UnguessableToken& window_id) {
  VideoWindowInfo* w = FindVideoWindowInfo(window_id);
  if (!w) {
    LOG(WARNING) << __func__ << " failed to find video window for "
                 << window_id;
    return;
  }
  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowDestroyed(w->owner_widget_, window_id));
  RemoveVideoWindowInfo(window_id);
}

void VideoWindowControllerImpl::BeginOverlayProcessor(gpu::SurfaceHandle h) {
  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end())
    return;

  std::set<base::UnguessableToken>& hidden_candidate = hidden_candidate_[w];
  hidden_candidate.clear();

  for (auto& window : wl_it->second)
    if (window.visibility_.has_value() && window.visibility_.value())
      hidden_candidate.insert(window.id_);
}

void VideoWindowControllerImpl::EndOverlayProcessor(gpu::SurfaceHandle h) {
  gfx::AcceleratedWidget w = static_cast<gfx::AcceleratedWidget>(h);

  auto wl_it = video_windows_.find(w);
  if (wl_it == video_windows_.end())
    return;

  std::set<base::UnguessableToken>& hidden = hidden_candidate_[w];
  for (auto id : hidden)
    SetVideoWindowVisibility(id, false);
}

}  // namespace ui
