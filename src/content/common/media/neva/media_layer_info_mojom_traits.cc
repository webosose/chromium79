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
#include "content/common/media/neva/media_layer_info_mojom_traits.h"

namespace mojo {

// static
bool StructTraits<
    content::mojom::MediaLayerInfoDataView,
    content::MediaLayerInfo>::Read(content::mojom::MediaLayerInfoDataView data,
                                   content::MediaLayerInfo* out_info) {
  base::UnguessableToken token;
  std::string id;
  if (!data.ReadOverlayPlaneToken(&token))
    return false;
  if (!data.ReadMediaLayerId(&id))
    return false;

  out_info->overlay_plane_token_ = token;
  out_info->media_layer_id_ = id;

  return true;
}

}  // namespace mojo
