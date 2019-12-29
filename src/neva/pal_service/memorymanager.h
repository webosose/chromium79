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

#ifndef NEVA_PAL_SERVICE_MEMORYMANAGER_H_
#define NEVA_PAL_SERVICE_MEMORYMANAGER_H_

#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "neva/pal_service/public/mojom/memorymanager.mojom.h"

namespace pal {

class MemoryManagerDelegate;

class MemoryManagerImpl : public mojom::MemoryManager {
 public:
  MemoryManagerImpl();
  ~MemoryManagerImpl() override;

  void AddBinding(mojom::MemoryManagerRequest request);

  // mojom::MemoryManager
  void GetMemoryStatus(GetMemoryStatusCallback callback) override;
  void Subscribe(SubscribeCallback callback) override;

private:
  void OnLevelChanged(const std::string& json);

  std::unique_ptr<MemoryManagerDelegate> delegate_;

  mojo::BindingSet<mojom::MemoryManager> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::MemoryManagerListener> listeners_;

  DISALLOW_COPY_AND_ASSIGN(MemoryManagerImpl);
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_MEMORYMANAGER_H_
