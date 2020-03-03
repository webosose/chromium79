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
#ifndef OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_HOST_IMPL_H_
#define OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_HOST_IMPL_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/platform_window/neva/mojo/video_window_controller.mojom.h"
#include "ui/platform_window/neva/video_window_controller_host.h"

namespace ui {
class OzoneGpuPlatformSupportHost;

/* VideoWindowControllerHostImpl lives in browser process and it communicates with
 * VideoWindowControllerImpl of gpu process. Main role of theses two class is
 * managing VideoWindows which are created by the native platform.
 * VideoWindow needs AcceleratedWidget and VideoWindow lays under the
 * AcceleratedWidget.
 */
class VideoWindowControllerHostImpl : public VideoWindowControllerHost,
                                      public GpuPlatformSupportHost {
 public:
  VideoWindowControllerHostImpl(OzoneGpuPlatformSupportHost* proxy);
  ~VideoWindowControllerHostImpl() override;

  // GpuPlatformSupportHost:
  void OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      base::RepeatingCallback<void(IPC::Message*)> sender) override {}
  void OnChannelDestroyed(int host_id) override {}
  void OnGpuServiceLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> host_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuHostBindInterfaceCallback binder,
      GpuHostTerminateCallback terminate_callback) override {}
  void OnMessageReceived(const IPC::Message& message) override {}

  ui::mojom::VideoWindowController* GetControllerRemote() override;

 private:
  OzoneGpuPlatformSupportHost* proxy_;
  GpuHostBindInterfaceCallback bind_interface_;
  mojo::Remote<ui::mojom::VideoWindowController> controller_;
  DISALLOW_COPY_AND_ASSIGN(VideoWindowControllerHostImpl);
};

}  // namespace ui

#endif  // OZONE_MEDIA_VIDEO_WINDOW_CONTROLLER_HOST_IMPL_H_
