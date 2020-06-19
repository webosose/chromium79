// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/system_media_controls/webos/system_media_controls_stub.h"

#include "base/memory/singleton.h"

namespace system_media_controls {

namespace internal {

// static
SystemMediaControlsStub* SystemMediaControlsStub::GetInstance() {
  return base::Singleton<SystemMediaControlsStub>::get();
}

SystemMediaControlsStub::SystemMediaControlsStub() = default;

SystemMediaControlsStub::~SystemMediaControlsStub() = default;

}  // namespace internal

}  // namespace system_media_controls
