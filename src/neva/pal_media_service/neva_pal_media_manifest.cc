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

#include "neva/pal_media_service/neva_pal_media_manifest.h"

#include <set>

#include "base/no_destructor.h"
#include "neva/pal_media_service/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/manifest_builder.h"

namespace pal_media {

const service_manager::Manifest& GetNevaPalMediaManifest() {
  static base::NoDestructor<service_manager::Manifest> manifest{
      service_manager::ManifestBuilder()
          .WithServiceName(pal_media::mojom::kServiceName)
          .WithDisplayName("PalMedia (PAL_MEDIA process)")
          .ExposeCapability("neva:media_player",
                            std::set<const char*>{
                                "pal_media.mojom.MediaPlayerProvider",
                            })
          .Build()};
  return *manifest;
}

}  // namespace pal_media
