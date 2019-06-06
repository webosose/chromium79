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

#include "neva/pal_service/system_servicebridge.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "neva/pal_service/pal_platform_factory.h"
#include "neva/pal_service/system_servicebridge_delegate.h"

namespace pal {

// SystemServiceBridgeImpl

SystemServiceBridgeImpl::SystemServiceBridgeImpl()
  : weak_factory_(this) {
}

SystemServiceBridgeImpl::~SystemServiceBridgeImpl() {
}

void SystemServiceBridgeImpl::Connect(const std::string& appid,
                                      ConnectCallback callback) {
  if (delegate_) {
    std::move(callback).Run(
        mojom::SystemServiceBridgeClientAssociatedRequest());
    return;
  }

  delegate_ = PlatformFactory::Get()->CreateSystemServiceBridgeDelegate(
      appid,
      base::BindRepeating(&SystemServiceBridgeImpl::OnResponse,
                          weak_factory_.GetWeakPtr()));
  std::move(callback).Run(mojo::MakeRequest(&(client_)));
}

void SystemServiceBridgeImpl::Call(const std::string& uri,
                                   const std::string& payload) {
  if (delegate_)
    delegate_->Call(uri, payload);
}

void SystemServiceBridgeImpl::Cancel() {
  if (delegate_)
    delegate_->Cancel();
}

void SystemServiceBridgeImpl::OnResponse(mojom::ResponseStatus status,
                                         const std::string& payload) {
  if (client_)
    client_->Response(status, payload);
}

// SystemServiceBridgeProviderImpl

SystemServiceBridgeProviderImpl::SystemServiceBridgeProviderImpl() {
}

SystemServiceBridgeProviderImpl::~SystemServiceBridgeProviderImpl() {
}

void SystemServiceBridgeProviderImpl::AddBinding(
    mojom::SystemServiceBridgeProviderRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void SystemServiceBridgeProviderImpl::GetSystemServiceBridge(
    mojom::SystemServiceBridgeRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<SystemServiceBridgeImpl>(),
      std::move(request));
}

}  // namespace pal
