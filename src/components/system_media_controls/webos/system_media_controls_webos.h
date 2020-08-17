// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_WEBOS_H_
#define COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_WEBOS_H_

#include <string>

#include "base/component_export.h"
#include "base/neva/webos/luna_service_client.h"
#include "base/observer_list.h"
#include "base/unguessable_token.h"
#include "components/system_media_controls/system_media_controls.h"

namespace system_media_controls {

class SystemMediaControlsObserver;

namespace internal {

class COMPONENT_EXPORT(SYSTEM_MEDIA_CONTROLS) SystemMediaControlsWebOS
    : public SystemMediaControls {
 public:
  static SystemMediaControlsWebOS* GetInstance();

  SystemMediaControlsWebOS();
  ~SystemMediaControlsWebOS() override;

  // SystemMediaControls implementation.
  void AddObserver(SystemMediaControlsObserver* observer) override;
  void RemoveObserver(SystemMediaControlsObserver* observer) override;
  void SetEnabled(bool enabled) override;
  void SetIsNextEnabled(bool value) override;
  void SetIsPreviousEnabled(bool value) override;
  void SetIsPlayEnabled(bool value) override;
  void SetIsPauseEnabled(bool value) override;
  void SetIsStopEnabled(bool value) override;
  void SetPlaybackStatus(PlaybackStatus value) override;
  void SetTitle(const base::string16& value) override;
  void SetArtist(const base::string16& value) override;
  void SetAlbum(const base::string16& value) override;
  void SetThumbnail(const SkBitmap& bitmap) override;
  void ClearThumbnail() override;
  void ClearMetadata() override;
  void UpdateDisplay() override;
  void SetMediaSessionId(
      const base::Optional<base::UnguessableToken>& session_id) override;

 private:
   enum class MediaKeyEvent {
    kUnsupported = 0,
    kPlay,
    kPause,
    kNext,
    kPrevious,
    kMute,
    kUnmute,
  };

  enum class ServiceReadyStatus {
    kNone,
    kRequested,
    kPending,
    kCompleted,
  };

  // Registers media session with MCS
  bool RegisterMediaSession(const std::string& session_id);

  // Unregisters media session with MCS
  void UnregisterMediaSession();

  // Activates media session with MCS
  bool ActivateMediaSession(const std::string& session_id);

  // Deactivates media session with MCS
  void DeactivateMediaSession();

  // Sets the current Playback Status to MCS.
  void SetPlaybackStatusInternal(const std::string& play_status);

  // Sets a value on the Metadata property and sends to MCS if necessary.
  void SetMetadataPropertyInternal(const std::string& property,
                                   const base::string16& value);

  void HandleMediaKeyEvent(const std::string& payload);

  void CheckReplyStatusMessage(const std::string& message);
  std::string GetAppIdFromSession(const base::UnguessableToken& request_id);

  void HandleMediaKeyEventInternal(const std::string& key_event);

  // True if we have registered to com.webos.service.mediacontroller service.
  bool registered_ = false;

  ServiceReadyStatus service_ready_status_ = ServiceReadyStatus::kNone;

  std::string application_id_;
  std::string session_id_;

  LSMessageToken subscribe_key_ = 0;
  std::unique_ptr<base::LunaServiceClient> luna_service_client_;

  base::ObserverList<SystemMediaControlsObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(SystemMediaControlsWebOS);
};

}  // namespace internal

}  // namespace system_media_controls

#endif  // COMPONENTS_SYSTEM_MEDIA_CONTROLS_WEBOS_SYSTEM_MEDIA_CONTROLS_WEBOS_H_
