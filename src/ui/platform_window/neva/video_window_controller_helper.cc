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

#include "ui/platform_window/neva/video_window_controller_helper.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif // defined(USE_OZONE)

#include "base/bind.h"
#include "ozone/platform/messages.h"

namespace ui {

VideoWindowControllerHostHelper::VideoWindowControllerHostHelper(
    VideoWindowControllerHost::Client* client)
    : client_(client)
#if defined(USE_OZONE)
      ,
      controller_(OzonePlatform::GetInstance()->GetVideoWindowControllerHost())
#endif  // defined(USE_OZONE)
{
  if (!controller_)
    return;

  controller_->RegisterClient(client_);
}

VideoWindowControllerHostHelper::~VideoWindowControllerHostHelper() {
  if (!controller_)
    return;

  controller_->UnRegisterClient(client_);
}

base::UnguessableToken VideoWindowControllerHostHelper::CreateVideoWindow(
    gfx::AcceleratedWidget owner) {
  if (!controller_)
    return base::UnguessableToken::Null();

  return controller_->CreateVideoWindow(client_, owner);
}

void VideoWindowControllerHostHelper::DestroyVideoWindow(
    const base::UnguessableToken& window_id) {
  if (!controller_)
    return;

  controller_->DestroyVideoWindow(client_, window_id);
}

std::string VideoWindowControllerHostHelper::GetNativeLayerId(
    const base::UnguessableToken& window_id) const {
  if (!controller_)
    return "";

  return controller_->GetNativeLayerId(window_id);
}

}  // namespace ui
