// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_STUB_H_
#define COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_STUB_H_

#include <string>

#include "base/component_export.h"
#include "components/system_media_controls/system_media_controls.h"

namespace system_media_controls {

class SystemMediaControlsObserver;

namespace internal {

class COMPONENT_EXPORT(SYSTEM_MEDIA_CONTROLS) SystemMediaControlsStub
    : public SystemMediaControls {
 public:
  static SystemMediaControlsStub* GetInstance();

  SystemMediaControlsStub();
  ~SystemMediaControlsStub() override;

  // SystemMediaControls implementation.
  void AddObserver(SystemMediaControlsObserver* observer) override {}
  void RemoveObserver(SystemMediaControlsObserver* observer) override {}
  void SetEnabled(bool enabled) override {}
  void SetIsNextEnabled(bool value) override {}
  void SetIsPreviousEnabled(bool value) override {}
  void SetIsPlayEnabled(bool value) override {}
  void SetIsPauseEnabled(bool value) override {}
  void SetIsStopEnabled(bool value) override {}
  void SetPlaybackStatus(PlaybackStatus value) override {}
  void SetTitle(const base::string16& value) override {}
  void SetArtist(const base::string16& value) override {}
  void SetAlbum(const base::string16& value) override {}
  void SetThumbnail(const SkBitmap& bitmap) override {}
  void ClearThumbnail() override {}
  void ClearMetadata() override {}
  void UpdateDisplay() override {}
  void SetMediaSessionId(
      const base::Optional<base::UnguessableToken>& session_id) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemMediaControlsStub);
};

}  // namespace internal

}  // namespace system_media_controls

#endif  // COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_STUB_H_
