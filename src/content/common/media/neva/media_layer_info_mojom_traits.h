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
#ifndef CONTENT_COMMON_MEDIA_NEVA_MEDIA_LAYER_INFO_MOJOM_TRAITS_H_
#define CONTENT_COMMON_MEDIA_NEVA_MEDIA_LAYER_INFO_MOJOM_TRAITS_H_

#include <string>

#include "base/unguessable_token.h"
#include "content/common/media/neva/media_layer_info.h"
#include "content/common/media/neva/media_layer_info.mojom.h"
#include "mojo/public/cpp/base/unguessable_token_mojom_traits.h"

namespace mojo {

template <>
class StructTraits<content::mojom::MediaLayerInfoDataView,
                   content::MediaLayerInfo> {
 public:
  static base::UnguessableToken overlay_plane_token(
      const content::MediaLayerInfo& info) {
    return info.overlay_plane_token_;
  }
  static std::string media_layer_id(const content::MediaLayerInfo& info) {
    return info.media_layer_id_;
  }

  static bool Read(content::mojom::MediaLayerInfoDataView data,
                   content::MediaLayerInfo* out_info);
};

}  // namespace mojo

#endif  // CONTENT_COMMON_MEDIA_NEVA_MEDIA_LAYER_INFO_MOJOM_TRAITS_H_
