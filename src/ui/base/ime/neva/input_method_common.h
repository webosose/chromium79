// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef UI_BASE_IME_NEVA_INPUT_METHOD_COMMON_H_
#define UI_BASE_IME_NEVA_INPUT_METHOD_COMMON_H_

#include "ui/base/ime/neva/input_content_type.h"

namespace ui {

// The text input information for handling IME on the UI side.
struct TextInputInfo {
  TextInputInfo() = default;
  ~TextInputInfo() = default;
  // Type of the input field.
  ui::InputContentType type = ui::InputContentType::INPUT_CONTENT_TYPE_NONE;

  // The flags of input field (autocorrect, autocomplete, etc.)
  int flags = 0;

  // Max input length for text input
  int max_length = 0;
};

enum class ImeHiddenType {
  // Only hide ime without deactivating
  kHide,
  // Deactivate ime
  kDeactivate
};

}  // namespace ui

#endif  // UI_BASE_IME_NEVA_INPUT_METHOD_COMMON_H_
