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

#ifndef UI_BASE_IME_NEVA_INPUT_CONTENT_TYPE_H_
#define UI_BASE_IME_NEVA_INPUT_CONTENT_TYPE_H_

namespace ui {

enum class InputContentType {
  INPUT_CONTENT_TYPE_NONE,
  INPUT_CONTENT_TYPE_TEXT,
  INPUT_CONTENT_TYPE_PASSWORD,
  INPUT_CONTENT_TYPE_SEARCH,
  INPUT_CONTENT_TYPE_EMAIL,
  INPUT_CONTENT_TYPE_NUMBER,
  INPUT_CONTENT_TYPE_TELEPHONE,
  INPUT_CONTENT_TYPE_URL,
  INPUT_CONTENT_TYPE_DATE,
  INPUT_CONTENT_TYPE_DATE_TIME,
  INPUT_CONTENT_TYPE_DATE_TIME_LOCAL,
  INPUT_CONTENT_TYPE_MONTH,
  INPUT_CONTENT_TYPE_TIME,
  INPUT_CONTENT_TYPE_WEEK,
  INPUT_CONTENT_TYPE_TEXT_AREA,
  INPUT_CONTENT_TYPE_CONTENT_EDITABLE,
  INPUT_CONTENT_TYPE_DATE_TIME_FIELD,
  INPUT_CONTENT_TYPE_MAX,
};

}  // namespace ui

#endif  // UI_BASE_IME_NEVA_INPUT_CONTENT_TYPE_H_
