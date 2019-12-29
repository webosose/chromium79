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
#ifndef CONTENT_COMMON_MEDIA_NEVA_MEDIA_LAYER_INFO_H_
#define CONTENT_COMMON_MEDIA_NEVA_MEDIA_LAYER_INFO_H_

#include <string>

#include "base/unguessable_token.h"

namespace content {

/* MediaLayerInfo contains two IDs.
 * overlay_plane_token - This is used in chromium side to update video window
 *  geometry so it stored in VideoFrame and passed to GPU overlay processor
 *  through VideoHoleDrawQuad.
 * media_layer_id - This is used in target platform (e.g webos) to identify
 *  which video window platform-pipeline draws videoframe on
 */
struct MediaLayerInfo {
  base::UnguessableToken overlay_plane_token_;
  std::string media_layer_id_;
};

}  // namespae content
#endif
