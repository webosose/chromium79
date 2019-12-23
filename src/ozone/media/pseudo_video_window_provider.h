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
#ifndef OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_
#define OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_

#include <map>
#include <string>

#include "base/cancelable_callback.h"
#include "base/unguessable_token.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ozone/media/video_window_provider.h"

namespace ui {
class VideoWindowSupport;

class PseudoVideoWindowProvider : public VideoWindowProvider {
 public:
  PseudoVideoWindowProvider(VideoWindowSupport*);
  ~PseudoVideoWindowProvider() override;
  void CreateNativeVideoWindow(gfx::AcceleratedWidget w,
                               const base::UnguessableToken& id,
                               WindowEventCb cb) override;
  void DestroyNativeVideoWindow(const base::UnguessableToken& id) override;
  std::string GetNativeVideoWindowName(
      const base::UnguessableToken& id) override;
  void NativeVideoWindowGeometryChanged(const base::UnguessableToken& window_id,
                                        const gfx::Rect& rect) override;
  void NativeVideoWindowVisibilityChanged(
      const base::UnguessableToken& window_id,
      bool visibility) override;

 private:
  struct PseudoVideoWindow;
  void UpdateNativeVideoWindowGeometry(const base::UnguessableToken& window_id);
  PseudoVideoWindow* FindWindow(const base::UnguessableToken& window_id);

  VideoWindowSupport* support_;

  std::map<base::UnguessableToken, std::unique_ptr<PseudoVideoWindow>>
      pseudo_windows_;
};

}  // namespace ui
#endif  // OZONE_MEDIA_PSEUDO_VIDEO_WINDOW_PROVIDER_H_
