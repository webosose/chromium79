// Copyright 2017 LG Electronics, Inc.
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

#ifndef NEVA_INJECTION_NETWORK_NETWORK_ERROR_PAGE_CONTROLLER_INJECTION_H_
#define NEVA_INJECTION_NETWORK_NETWORK_ERROR_PAGE_CONTROLLER_INJECTION_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "neva/injection/network/network_error_page_controller_export.h"
#include "neva/pal_service/public/mojom/network_error_page_controller.mojom.h"

namespace content {
class RenderFrame;
}

namespace injections {

class NETWORK_ERROR_PAGE_CONTROLLER_EXPORT NetworkErrorPageControllerInjection
    : public gin::Wrappable<NetworkErrorPageControllerInjection> {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static bool Install(content::RenderFrame* render_frame);
  static void Uninstall(content::RenderFrame* render_frame);

 private:
  static const char kInjectionName[];

  explicit NetworkErrorPageControllerInjection();
  ~NetworkErrorPageControllerInjection() override;

  // Execute a "NETWORK SETTINGS" button click.
  bool SettingsButtonClick(const gin::Arguments& args);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  mojo::Remote<pal::mojom::NetworkErrorPageController> controller_;

  DISALLOW_COPY_AND_ASSIGN(NetworkErrorPageControllerInjection);
};

}  // namespace injections

#endif  // NEVA_INJECTION_NETWORK_NETWORK_ERROR_PAGE_CONTROLLER_INJECTION_H_
