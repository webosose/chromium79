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
#ifndef UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HELPER_H_
#define UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HELPER_H_

#include <map>
#include <set>

#include "base/optional.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"
#include "base/unguessable_token.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/neva/video_window_controller_host.h"

namespace ui {

class VideoWindowControllerHostHelper {
 public:
  VideoWindowControllerHostHelper(VideoWindowControllerHost::Client* client);
  ~VideoWindowControllerHostHelper();
  base::UnguessableToken CreateVideoWindow(gfx::AcceleratedWidget owner);
  void DestroyVideoWindow(const base::UnguessableToken&);
  std::string GetNativeLayerId(const base::UnguessableToken& window_id) const;

 private:
  VideoWindowControllerHost::Client* client_ = nullptr;
  VideoWindowControllerHost* controller_ = nullptr;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HELPER_H_
