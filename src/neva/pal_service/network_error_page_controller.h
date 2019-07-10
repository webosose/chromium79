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

#ifndef NEVA_PAL_SERVICE_NETWORK_ERROR_PAGE_CONTROLLER_H_
#define NEVA_PAL_SERVICE_NETWORK_ERROR_PAGE_CONTROLLER_H_

#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "neva/pal_service/public/mojom/network_error_page_controller.mojom.h"

namespace pal {

class NetworkErrorPageControllerDelegate;

class NetworkErrorPageControllerImpl
    : public mojom::NetworkErrorPageController {
 public:
  NetworkErrorPageControllerImpl();
  ~NetworkErrorPageControllerImpl() override;

  void AddBinding(mojom::NetworkErrorPageControllerRequest request);

  // mojom::NetworkErrorPageController
  void Connect() override;
  void LaunchNetworkSettings(int target_id) override;

 private:
  std::unique_ptr<NetworkErrorPageControllerDelegate> delegate_;

  mojo::BindingSet<mojom::NetworkErrorPageController> bindings_;

  DISALLOW_COPY_AND_ASSIGN(NetworkErrorPageControllerImpl);
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_NETWORK_ERROR_PAGE_CONTROLLER_H_
