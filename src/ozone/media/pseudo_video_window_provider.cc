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
}  // namespace

struct PseudoVideoWindowProvider::PseudoVideoWindow : public ui::VideoWindow {
  PseudoVideoWindow() = default;
  ~PseudoVideoWindow() = default;
  WindowEventCb window_event_cb_;
  base::CancelableOnceCallback<void()> notify_geometry_cb_;
  gfx::Rect rect_;
  base::Time last_updated_ = base::Time::Now();
};

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
  auto result =
      pseudo_windows_.emplace(id, std::make_unique<PseudoVideoWindow>());
  if (!result.second) {
    LOG(ERROR) << __func__ << " failed to inster PseudoVideoWindow for " << id;
    return;
  }
  auto& window = result.first->second;
  window->window_id_ = id;
  window->native_window_name_ = "window_id_dummy";
  window->window_event_cb_ = cb;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(cb, id, ui::VideoWindowProvider::Event::kCreated));
}

void PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry(
    const base::UnguessableToken& window_id) {
  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (!win->notify_geometry_cb_.IsCancelled())
    win->notify_geometry_cb_.Cancel();

  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowGeometryChanged(window_id, win->rect_));
  win->last_updated_ = base::Time::Now();
}

void PseudoVideoWindowProvider::NativeVideoWindowGeometryChanged(
    const base::UnguessableToken& window_id,
    const gfx::Rect& rect) {
  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (win->rect_ == rect)
    return;
  win->rect_ = rect;

  // callback is already scheduled!
  if (!win->notify_geometry_cb_.IsCancelled()) {
    return;
  }

  if (base::Time::Now() - win->last_updated_ <
      base::TimeDelta::FromMilliseconds(kMinVideoGeometryUpdateInterval)) {
    win->notify_geometry_cb_.Reset(base::BindOnce(
        &PseudoVideoWindowProvider::UpdateNativeVideoWindowGeometry,
        base::Unretained(this), window_id));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, win->notify_geometry_cb_.callback(),
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

  PseudoVideoWindow* win = FindWindow(window_id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << window_id;
    return;
  }
  if (!win->notify_geometry_cb_.IsCancelled()) {
    UpdateNativeVideoWindowGeometry(window_id);
  }

  support_->SendVideoWindowMessage(
      new WaylandDisplay_VideoWindowVisibilityChanged(window_id, visibility));
}

void PseudoVideoWindowProvider::DestroyNativeVideoWindow(
    const base::UnguessableToken& id) {
  PseudoVideoWindow* win = FindWindow(id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << id;
    return;
  }
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(win->window_event_cb_, id,
                                ui::VideoWindowProvider::Event::kDestroyed));
  pseudo_windows_.erase(id);
}

std::string PseudoVideoWindowProvider::GetNativeVideoWindowName(
    const base::UnguessableToken& id) {
  PseudoVideoWindow* win = FindWindow(id);
  if (!win) {
    LOG(ERROR) << __func__ << " failed to find info for " << id;
    return "";
  }
  return win->native_window_name_;
}

PseudoVideoWindowProvider::PseudoVideoWindow*
PseudoVideoWindowProvider::FindWindow(const base::UnguessableToken& window_id) {
  auto it = pseudo_windows_.find(window_id);
  if (it == pseudo_windows_.end())
    return nullptr;
  return it->second.get();
}
}  // namespace ui
