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

#include "neva/pal_service/pal_service.h"

#include "neva/pal_service/memorymanager.h"
#include "neva/pal_service/network_error_page_controller.h"
#include "neva/pal_service/sample.h"
#include "neva/pal_service/system_servicebridge.h"

namespace pal {

PalService::PalService(service_manager::mojom::ServiceRequest request)
    : service_binding_(this, std::move(request)) {
}

PalService::~PalService() {
}

void PalService::OnStart() {
  registry_.AddInterface<mojom::MemoryManager>(
      base::BindRepeating(
          &PalService::BindMemoryManagerRequest,
          base::Unretained(this)));

  registry_.AddInterface<mojom::Sample>(
      base::BindRepeating(
          &PalService::BindSampleRequest,
          base::Unretained(this)));

  registry_.AddInterface<mojom::SystemServiceBridgeProvider>(
      base::BindRepeating(
          &PalService::BindSystemServiceBridgeProviderRequest,
          base::Unretained(this)));

  registry_.AddInterface<mojom::NetworkErrorPageController>(
      base::BindRepeating(&PalService::BindNetworkErrorPageControllerRequest,
                          base::Unretained(this)));
}

void PalService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void PalService::BindMemoryManagerRequest(mojom::MemoryManagerRequest request) {
  if (!memorymanager_impl_)
    memorymanager_impl_ = std::make_unique<MemoryManagerImpl>();
  memorymanager_impl_->AddBinding(std::move(request));
}

void PalService::BindSampleRequest(mojom::SampleRequest request) {
  if (!sample_impl_)
    sample_impl_ = std::make_unique<SampleImpl>();
  sample_impl_->AddBinding(std::move(request));
}

void PalService::BindSystemServiceBridgeProviderRequest(
    mojom::SystemServiceBridgeProviderRequest request) {
  if (!system_servicebridge_provider_impl_) {
    system_servicebridge_provider_impl_ =
        std::make_unique<SystemServiceBridgeProviderImpl>();
  }
  system_servicebridge_provider_impl_->AddBinding(std::move(request));
}

void PalService::BindNetworkErrorPageControllerRequest(
    mojom::NetworkErrorPageControllerRequest request) {
  if (!network_error_page_controller_impl_) {
    network_error_page_controller_impl_ =
        std::make_unique<NetworkErrorPageControllerImpl>();
  }
  network_error_page_controller_impl_->AddBinding(std::move(request));
}

}  // namespace pal
