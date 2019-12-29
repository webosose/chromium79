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

#ifndef NEVA_PAL_MEDIA_SERVICE_PAL_MEDIA_SERVICE_H_
#define NEVA_PAL_MEDIA_SERVICE_PAL_MEDIA_SERVICE_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "neva/pal_media_service/pal_media_service_export.h"
#include "neva/pal_media_service/public/mojom/media_player.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_binding.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace pal_media {

class MediaPlayerProviderImpl;

class PAL_MEDIA_SERVICE_EXPORT PalMediaService
    : public service_manager::Service {
 public:
  explicit PalMediaService(service_manager::mojom::ServiceRequest request);
  ~PalMediaService() override;

 private:
  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  void BindMediaPlayerProviderRequest(
      mojom::MediaPlayerProviderRequest request);

  service_manager::ServiceBinding service_binding_;
  service_manager::BinderRegistry registry_;
  std::unique_ptr<pal_media::MediaPlayerProviderImpl>
      media_player_provider_impl_;
};

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_PAL_MEDIA_SERVICE_H_
