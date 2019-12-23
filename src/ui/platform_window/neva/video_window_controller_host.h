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
#ifndef UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HOST_H_
#define UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HOST_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

/* VideoWindowControllerHost lives in browser process and it communicates with
 * VideoWindowController of gpu process. Main role of theses two class is
 * managing VideoWindows which are created by the native platform.
 * VideoWindow needs AcceleratedWidget and VideoWindow lays under the
 * AcceleratedWidget.
 */
class VideoWindowControllerHost {
 public:
  class Client {
   public:
    virtual void OnVideoWindowCreated(
        const base::UnguessableToken& window_id) = 0;
    virtual void OnVideoWindowDestroyed(
        const base::UnguessableToken& window_id) = 0;

    virtual void OnVideoWindowGeometryChanged(
        const base::UnguessableToken& window_id,
        const gfx::Rect& rect) = 0;
    virtual void OnVideoWindowVisibilityChanged(
        const base::UnguessableToken& window_id,
        bool visibility) = 0;
  };

  virtual ~VideoWindowControllerHost() {}

  virtual void RegisterClient(Client*) = 0;
  virtual void UnRegisterClient(Client*) = 0;

  virtual base::UnguessableToken CreateVideoWindow(
      Client*,
      gfx::AcceleratedWidget owner) = 0;
  virtual void DestroyVideoWindow(Client*,
                                  const base::UnguessableToken& window_id) = 0;
  virtual std::string GetNativeLayerId(
      const base::UnguessableToken& window_id) = 0;
};
}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_CONTROLLER_HOST_H_
