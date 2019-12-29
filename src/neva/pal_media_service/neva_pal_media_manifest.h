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

#ifndef NEVA_PAL_MEDIA_SERVICE_NEVA_PAL_MEDIA_MANIFEST_H_
#define NEVA_PAL_MEDIA_SERVICE_NEVA_PAL_MEDIA_MANIFEST_H_

#include "neva/pal_media_service/pal_media_service_export.h"
#include "services/service_manager/public/cpp/manifest.h"

namespace pal_media {

PAL_MEDIA_SERVICE_EXPORT const service_manager::Manifest&
GetNevaPalMediaManifest();

}  // namespace pal_media

#endif  // NEVA_PAL_MEDIA_SERVICE_NEVA_PAL_MEDIA_MANIFEST_H_
