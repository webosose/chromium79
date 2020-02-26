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
#ifndef OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_
#define OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_

#include <wayland-client.h>
#include <wayland-webos-foreign-client-protocol.h>

#include <map>
#include <string>

#include "base/cancelable_callback.h"
#include "base/single_thread_task_runner.h"
#include "base/unguessable_token.h"
#include "ozone/media/video_window_provider.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

namespace ui {
class VideoWindowSupport;

class ForeignVideoWindowProvider : public VideoWindowProvider {
 public:
  ForeignVideoWindowProvider(VideoWindowSupport*);
  ~ForeignVideoWindowProvider() override;

  static void HandleExportedWindowAssigned(
      void* data,
      struct wl_webos_exported* webos_exported,
      const char* window_id,
      uint32_t exported_type);
  void OnCreatedForeignWindow(struct wl_webos_exported* webos_exported,
                              const char* window_id,
                              ui::ForeignWindowType type);

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
  struct ForeignVideoWindow;
  void UpdateNativeVideoWindowGeometry(const base::UnguessableToken& window_id);
  ForeignVideoWindow* FindWindow(struct wl_webos_exported* webos_exported);
  ForeignVideoWindow* FindWindow(const base::UnguessableToken& id);

  VideoWindowSupport* support_;

  std::map<base::UnguessableToken, std::unique_ptr<ForeignVideoWindow>>
      foreign_windows_;
  std::map<std::string, base::UnguessableToken> native_id_to_window_id_;

  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

}  // namespace ui
#endif  // OZONE_MEDIA_FOREIGN_VIDEO_WINDOW_PROVIDER_H_
