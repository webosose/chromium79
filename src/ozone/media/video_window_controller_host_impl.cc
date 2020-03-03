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
#include "content/public/browser/gpu_service_registry.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ozone/platform/messages.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"

namespace ui {

VideoWindowControllerHostImpl::VideoWindowControllerHostImpl(
    OzoneGpuPlatformSupportHost* proxy)
    : proxy_(proxy) {
  proxy_->RegisterHandler(this);
}

VideoWindowControllerHostImpl::~VideoWindowControllerHostImpl() = default;

ui::mojom::VideoWindowController*
VideoWindowControllerHostImpl::GetControllerRemote() {
  if (!controller_) {
    mojo::PendingRemote<ui::mojom::VideoWindowController> controller_remote;
    content::BindInterfaceInGpuProcess(
        controller_remote.InitWithNewPipeAndPassReceiver());
    controller_.Bind(std::move(controller_remote));
  }
  return controller_.get();
}
}  // namespace ui
