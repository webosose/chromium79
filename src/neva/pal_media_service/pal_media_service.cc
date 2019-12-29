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

#include "neva/pal_media_service/mediaplayer_base_impl.h"
#include "neva/pal_media_service/pal_media_service.h"

namespace pal_media {

PalMediaService::PalMediaService(service_manager::mojom::ServiceRequest request)
    : service_binding_(this, std::move(request)) {}

PalMediaService::~PalMediaService() {}

void PalMediaService::OnStart() {
  registry_.AddInterface<mojom::MediaPlayerProvider>(
      base::BindRepeating(&PalMediaService::BindMediaPlayerProviderRequest,
                          base::Unretained(this)));
}

void PalMediaService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void PalMediaService::BindMediaPlayerProviderRequest(
    mojom::MediaPlayerProviderRequest request) {
  if (!media_player_provider_impl_)
    media_player_provider_impl_ = std::make_unique<MediaPlayerProviderImpl>();
  media_player_provider_impl_->AddBinding(std::move(request));
}

}  // namespace pal_media
