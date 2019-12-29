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
#include "ozone/media/pseudo_video_window_provider.h"

#include <memory>
#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "ozone/platform/messages.h"
#include "ui/gfx/geometry/rect.h"
#include "ozone/media/video_window_controller_impl.h"

namespace ui {

namespace {
const int kMinVideoGeometryUpdateInterval = 200;  // milliseconds
}

PseudoVideoWindowProvider::PseudoVideoWindow::PseudoVideoWindow() = default;

PseudoVideoWindowProvider::PseudoVideoWindow::~PseudoVideoWindow() = default;

std::unique_ptr<VideoWindowProvider> VideoWindowProvider::Create(
    VideoWindowSupport* support) {
  return std::make_unique<PseudoVideoWindowProvider>(support);
}

PseudoVideoWindowProvider::PseudoVideoWindowProvider(
    VideoWindowSupport* support)
    : support_(support) {}

PseudoVideoWindowProvider::~PseudoVideoWindowProvider() = default;

void PseudoVideoWindowProvider::CreateNativeVideoWindow(
    gfx::AcceleratedWidget w,
    const base::UnguessableToken& id,
    ui::VideoWindowProvider::WindowEventCb cb) {
  auto& window = pseudo_windows_[id];
  window.window_id_ = id;
  window.native_window_name_ = "window_id_dummy";
  window.window_event_cb_ = cb;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(cb, id, ui::VideoWindowProvider::Event::kCreated));
}

void PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry(
    const base::UnguessableToken& window_id) {
  auto it = pseudo_windows_.find(window_id);
  if (it == pseudo_windows_.end()) {
    LOG(ERROR) << __func__ << " failed to find pseudo window for " << window_id;
    return;
  }

  PseudoVideoWindow& w = it->second;
  if (!w.notify_geometry_cb_.IsCancelled())
    w.notify_geometry_cb_.Cancel();

  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowGeometryChanged(window_id, w.rect_));
  w.last_updated_ = base::Time::Now();
}

void PseudoVideoWindowProvider::NativeVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  auto it = pseudo_windows_.find(window_id);
  if (it == pseudo_windows_.end()) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }

  PseudoVideoWindow& win = it->second;

  if (win.rect_ == rect)
    return;
  win.rect_ = rect;

  // callback is already scheduled!
  if (!win.notify_geometry_cb_.IsCancelled()) {
    return;
  }

  if (base::Time::Now() - win.last_updated_ <
      base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval)) {
    win.notify_geometry_cb_.Reset(base::BindOnce(
        &PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry,
        base::Unretained(this), window_id));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, win.notify_geometry_cb_.callback(),
        base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval));
    return;
  }
  UpdateNativeVideoWindowGeometry(window_id);
}

void PseudoVideoWindowProvider::NativeVideoWindowVisibilityChanged(
    const base::UnguessableToken& window_id,
    bool visibility) {
  VLOG(1) << __func__ << " window_id=" << window_id
          << " visibility=" << visibility;

  auto win_it = pseudo_windows_.find(window_id);
  if (win_it == pseudo_windows_.end()) {
    LOG(ERROR) << __func__ << " failed to find windows for id=" << window_id;
    return;
  }
  PseudoVideoWindow& win = win_it->second;
  if (!win.notify_geometry_cb_.IsCancelled()) {
    UpdateNativeVideoWindowGeometry(window_id);
  }

  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowVisibilityChanged(window_id, visibility));
}

void PseudoVideoWindowProvider::DestroyNativeVideoWindow(
    const base::UnguessableToken& id) {
  auto it = pseudo_windows_.find(id);
  if (it != pseudo_windows_.end()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(it->second.window_event_cb_, id,
                                  ui::VideoWindowProvider::Event::kDestroyed));
    pseudo_windows_.erase(id);
  } else {
    LOG(WARNING) << __func__ << " failed to find video window id=" << id;
  }
}

std::string PseudoVideoWindowProvider::GetNativeVideoWindowName(
    const base::UnguessableToken& id) {
  return pseudo_windows_[id].native_window_name_;
}
}  // namespace ui
