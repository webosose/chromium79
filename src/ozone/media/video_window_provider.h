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
#ifndef OZONE_MEDIA_VIDEO_WINDOW_PROVIDER_H_
#define OZONE_MEDIA_VIDEO_WINDOW_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/unguessable_token.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

class VideoWindowSupport;

struct VideoWindow {
  base::UnguessableToken window_id_;
  std::string native_window_name_;
};

/* VideoWindowProvider is a interface to provide platform specific video overlay
 * window. Implementation needs to create video window and map requested id to
 * platform's video window. Also there should be platform wise window name which
 * can be used as identifier in platform-side.
 */
class VideoWindowProvider {
 public:
  enum class Event { kCreated, kDestroyed };

  static std::unique_ptr<VideoWindowProvider> Create(VideoWindowSupport*);

  using WindowEventCb =
      base::RepeatingCallback<void(const base::UnguessableToken&, Event)>;

  virtual void CreateNativeVideoWindow(gfx::AcceleratedWidget w,
                                       const base::UnguessableToken& id,
                                       WindowEventCb cb) = 0;
  virtual void DestroyNativeVideoWindow(const base::UnguessableToken& id) = 0;
  virtual std::string GetNativeVideoWindowName(
      const base::UnguessableToken& id) = 0;
  virtual void NativeVideoWindowGeometryChanged(
      const base::UnguessableToken& window_id,
      const gfx::Rect& rect) = 0;
  virtual void NativeVideoWindowVisibilityChanged(
      const base::UnguessableToken& window_id,
      bool visibility) = 0;
  virtual ~VideoWindowProvider() {}
};

}  // namespace ui
#endif  // OZONE_MEDIA_VIDEO_WINDOW_PROVIDER_H_
