// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/system_media_controls/webos/system_media_controls_webos.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/singleton.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/branding_buildflags.h"
#include "components/system_media_controls/system_media_controls_observer.h"
#include "components/system_media_controls/webos/system_media_controls_stub.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"
#include "media/base/bind_to_current_loop.h"
#include "services/media_session/public/cpp/features.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace system_media_controls {

// static
SystemMediaControls* SystemMediaControls::GetInstance() {
  if (!base::FeatureList::IsEnabled(
          media_session::features::kMediaControllerService))
    return internal::SystemMediaControlsStub::GetInstance();
  return internal::SystemMediaControlsWebOS::GetInstance();;
}

namespace internal {

namespace {

const char kAppId[] = "appId";
const char kMediaId[] = "mediaId";
const char kSubscribe[] = "subscribe";
const char kSubscribed[] = "subscribed";
const char kReturnValue[] = "returnValue";
const char kKeyEvent[] = "keyEvent";

const char kMediaMetaData[] = "mediaMetaData";
const char kMediaMetaDataTitle[] = "title";
const char kMediaMetaDataArtist[] = "artist";
const char kMediaMetaDataAlbum[] = "album";

const char kMediaPlayStatus[] = "playStatus";
const char kMediaPlayStatusStopped[] = "PLAYSTATE_STOPPED";
const char kMediaPlayStatusPaused[] = "PLAYSTATE_PAUSED";
const char kMediaPlayStatusPlaying[] = "PLAYSTATE_PLAYING";

const char kPlayEvent[] = "play";
const char kPauseEvent[] = "pause";
const char kNextEvent[] = "next";
const char kPreviousEvent[] = "previous";
const char kMuteEvent[] = "mute";
const char kUnmuteEvent[] = "unmute";

const char kRegisterMediaSession[] = "registerMediaSession";
const char kUnregisterMediaSession[] = "unregisterMediaSession";
const char kActivateMediaSession[] = "activateMediaSession";
const char kDeactivateMediaSession[] = "deactivateMediaSession";
const char kSetMediaMetaData[] = "setMediaMetaData";
const char kSetMediaPlayStatus[] = "setMediaPlayStatus";

}  // namespace

#define BIND_TO_CURRENT_LOOP(function) \
  (media::BindToCurrentLoop(           \
      base::BindRepeating(function, base::Unretained(this))))

// static
SystemMediaControlsWebOS* SystemMediaControlsWebOS::GetInstance() {
  return base::Singleton<SystemMediaControlsWebOS>::get();
}

SystemMediaControlsWebOS::SystemMediaControlsWebOS() = default;

SystemMediaControlsWebOS::~SystemMediaControlsWebOS() {
  UnregisterMediaSession();
}

void SystemMediaControlsWebOS::AddObserver(
    SystemMediaControlsObserver* observer) {
  observers_.AddObserver(observer);

  // If the service is already ready, inform the observer.
  if (registered_) {
    observer->OnServiceReady();
    service_ready_status_ = ServiceReadyStatus::kCompleted;
    return;
  }
  service_ready_status_ = ServiceReadyStatus::kPending;
}

void SystemMediaControlsWebOS::RemoveObserver(
    SystemMediaControlsObserver* observer) {
  observers_.RemoveObserver(observer);
}

void SystemMediaControlsWebOS::SetEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetIsNextEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetIsPreviousEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetIsPlayEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetIsPauseEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetIsStopEnabled(bool value) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetPlaybackStatus(PlaybackStatus value) {
  auto status = [&]() {
    switch (value) {
      case PlaybackStatus::kPlaying:
        return std::string(kMediaPlayStatusPlaying);
      case PlaybackStatus::kPaused:
        return std::string(kMediaPlayStatusPaused);
      case PlaybackStatus::kStopped:
        return std::string(kMediaPlayStatusStopped);
    }
    NOTREACHED() << __func__ << " Invalid playback status. session_id: "
                 << session_id_;
    return std::string("Unidentified");
  };

  SetPlaybackStatusInternal(status());
}

void SystemMediaControlsWebOS::SetTitle(const base::string16& value) {
  SetMetadataPropertyInternal(kMediaMetaDataTitle, value);
}

void SystemMediaControlsWebOS::SetArtist(const base::string16& value) {
  SetMetadataPropertyInternal(kMediaMetaDataArtist, value);
}

void SystemMediaControlsWebOS::SetAlbum(const base::string16& value) {
  SetMetadataPropertyInternal(kMediaMetaDataAlbum, value);
}

void SystemMediaControlsWebOS::SetThumbnail(const SkBitmap& bitmap) {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::ClearThumbnail() {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::ClearMetadata() {
  VLOG(1) << __func__;
  SetTitle(base::string16());
  SetArtist(base::string16());
  SetAlbum(base::string16());
}

void SystemMediaControlsWebOS::UpdateDisplay() {
  // TODO: Propogate the status to MCS after MCS provides such APIs
}

void SystemMediaControlsWebOS::SetMediaSessionId(
    const base::Optional<base::UnguessableToken>& session_id) {
  if (session_id.has_value())
    application_id_ = GetAppIdFromSession(session_id.value());

  if (application_id_.empty()) {
    LOG(ERROR) << __func__ << " Could not obtain app_id.";
    return;
  }

  if (!luna_service_client_.get()) {
    luna_service_client_.reset(
        new base::LunaServiceClient(application_id_));
  }


  if (!session_id_.empty()) {
    // Previous session is active. Deactivate it.
    UnregisterMediaSession();
  }

  if (!session_id.has_value()) {
    LOG(ERROR) << __func__ << "session_id not received.";
    return;
  }

  if (!RegisterMediaSession(session_id->ToString())) {
    LOG(ERROR) << __func__ << " Register session failed.";
    return;
  }

  if (!ActivateMediaSession(session_id->ToString())) {
    LOG(ERROR) << __func__ << " Activate session failed.";
    return;
  }

  session_id_ = session_id->ToString();
}

bool SystemMediaControlsWebOS::RegisterMediaSession(
    const std::string& session_id) {
  if (session_id.empty()) {
    LOG(ERROR) << __func__ << "Invalid session id.";
    return false;
  }

  Json::Value register_root;
  register_root[kMediaId] = session_id;
  register_root[kAppId] = application_id_;
  register_root[kSubscribe] = true;

  Json::FastWriter register_writer;
  std::string register_paylaod = register_writer.write(register_root);
  LOG(INFO) << __func__ << " payload: " << register_paylaod;

  luna_service_client_->Subscribe(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kRegisterMediaSession),
      register_paylaod, &subscribe_key_,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::HandleMediaKeyEvent));

  registered_ = true;
  return true;
}

void SystemMediaControlsWebOS::UnregisterMediaSession() {
  if (!registered_) {
    LOG(ERROR) << __func__ << " Session is already unregistered.";
    return;
  }

  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No registered session.";
    return;
  }

  Json::Value unregister_root;
  unregister_root[kMediaId] = session_id_;

  Json::FastWriter unregister_writer;
  std::string unregister_payload = unregister_writer.write(unregister_root);
  VLOG(1) << __func__ << " payload: " << unregister_payload;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kUnregisterMediaSession),
      unregister_payload,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::CheckReplyStatusMessage));

  luna_service_client_->Unsubscribe(subscribe_key_);

  registered_ = false;
  session_id_ = std::string();
}

bool SystemMediaControlsWebOS::ActivateMediaSession(
    const std::string& session_id) {
  if (session_id.empty()) {
    LOG(ERROR) << __func__ << "Invalid session id.";
    return false;
  }

  Json::Value activate_root;
  activate_root[kMediaId] = session_id;

  Json::FastWriter activate_writer;
  std::string activate_paylaod = activate_writer.write(activate_root);
  VLOG(1) << __func__ << " payload: " << activate_paylaod;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kActivateMediaSession),
      activate_paylaod,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::CheckReplyStatusMessage));

  return true;
}

void SystemMediaControlsWebOS::DeactivateMediaSession() {
  if (session_id_.empty()) {
    LOG(ERROR) << __func__ << " No active session.";
    return;
  }

  Json::Value deactivate_root;
  deactivate_root[kMediaId] = session_id_;

  Json::FastWriter deactivate_writer;
  std::string deactivate_payload = deactivate_writer.write(deactivate_root);
  VLOG(1) << __func__ << " payload: " << deactivate_payload;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kDeactivateMediaSession),
      deactivate_payload,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::CheckReplyStatusMessage));
}

void SystemMediaControlsWebOS::SetPlaybackStatusInternal(
    const std::string& play_status) {
  Json::Value playstatus_root;
  playstatus_root[kMediaId] = session_id_;
  playstatus_root[kMediaPlayStatus] = play_status;

  Json::FastWriter playstatus_writer;
  std::string playstatus_paylaod = playstatus_writer.write(playstatus_root);
  VLOG(1) << __func__ << " payload: " << playstatus_paylaod;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER,
          kSetMediaPlayStatus),
      playstatus_paylaod,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::CheckReplyStatusMessage));
}

void SystemMediaControlsWebOS::SetMetadataPropertyInternal(
    const std::string& property,
    const base::string16& value) {
  Json::Value metadata;
  metadata[property] = base::UTF16ToUTF8(value);

  Json::Value metadata_root;
  metadata_root[kMediaId] = session_id_;
  metadata_root[kMediaMetaData] = metadata;

  Json::FastWriter metadata_writer;
  std::string metadata_paylaod = metadata_writer.write(metadata_root);
  VLOG(1) << __func__ << " payload: " << metadata_paylaod;

  luna_service_client_->CallAsync(
      base::LunaServiceClient::GetServiceURI(
          base::LunaServiceClient::URIType::MEDIACONTROLLER, kSetMediaMetaData),
      metadata_paylaod,
      BIND_TO_CURRENT_LOOP(&SystemMediaControlsWebOS::CheckReplyStatusMessage));
}

void SystemMediaControlsWebOS::HandleMediaKeyEvent(const std::string& payload) {
  VLOG(1) << __func__ << " payload: " << payload;

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember(kReturnValue) &&
      response[kReturnValue].asBool() && response.isMember(kSubscribed) &&
      response[kSubscribed].asBool()) {
    if (response.isMember(kKeyEvent)) {
      HandleMediaKeyEventInternal(response[kKeyEvent].asString());
    } else {
      if (service_ready_status_ == ServiceReadyStatus::kPending) {
        for (SystemMediaControlsObserver& obs : observers_)
          obs.OnServiceReady();
        service_ready_status_ == ServiceReadyStatus::kCompleted;
      }
      LOG(INFO) << __func__ << " Successfully Registered with MCS, session_id: "
                            << session_id_;
    }
  } else {
    LOG(ERROR) << " Failed to Register with MCS, session_id: " << session_id_;
  }
}

void SystemMediaControlsWebOS::CheckReplyStatusMessage(
    const std::string& message) {
  Json::Value response;
  Json::Reader reader;
  if (!(reader.parse(message, response) && response.isMember(kReturnValue) &&
        response[kReturnValue].asBool())) {
    LOG(ERROR) << __func__ << " MCS call Failed. message: " << message
               << " session_id: " << session_id_;
  } else {
    VLOG(1) << __func__ << " MCS call Success. message: " << message
            << " session_id: " << session_id_;
  }
}

std::string SystemMediaControlsWebOS::GetAppIdFromSession(
    const base::UnguessableToken& request_id) {
  content::WebContents* web_contents =
      content::MediaSession::GetWebContentsFromRequestId(request_id);
  if (web_contents) {
    blink::mojom::RendererPreferences* renderer_prefs =
        web_contents->GetMutableRendererPrefs();
    if (renderer_prefs)
      return renderer_prefs->application_id;
  }
  return std::string();
}

void SystemMediaControlsWebOS::HandleMediaKeyEventInternal(
    const std::string& key_event) {
  VLOG(1) << __func__ << " key_event: " << key_event;

  static std::map<std::string, MediaKeyEvent> kEventKeyMap = {
    {kPlayEvent, SystemMediaControlsWebOS::MediaKeyEvent::kPlay},
    {kPauseEvent, SystemMediaControlsWebOS::MediaKeyEvent::kPause},
    {kNextEvent, SystemMediaControlsWebOS::MediaKeyEvent::kNext},
    {kPreviousEvent, SystemMediaControlsWebOS::MediaKeyEvent::kPrevious},
    {kMuteEvent, SystemMediaControlsWebOS::MediaKeyEvent::kMute},
    {kUnmuteEvent, SystemMediaControlsWebOS::MediaKeyEvent::kUnmute}
  };

  auto get_event_type = [&](const std::string& key) {
    std::map<std::string, MediaKeyEvent>::iterator it;
    it = kEventKeyMap.find(key);
    if (it != kEventKeyMap.end())
      return it->second;
    return SystemMediaControlsWebOS::MediaKeyEvent::kUnsupported;
  };

  MediaKeyEvent event_type = get_event_type(key_event);
  for (SystemMediaControlsObserver& obs : observers_) {
    switch (event_type) {
      case SystemMediaControlsWebOS::MediaKeyEvent::kPlay:
        obs.OnPlay();
        break;
      case SystemMediaControlsWebOS::MediaKeyEvent::kPause:
        obs.OnPause();
        break;
      case SystemMediaControlsWebOS::MediaKeyEvent::kNext:
        obs.OnNext();
        break;
      case SystemMediaControlsWebOS::MediaKeyEvent::kPrevious:
        obs.OnPrevious();
        break;
      case SystemMediaControlsWebOS::MediaKeyEvent::kMute:
        obs.OnMuteStateChanged(true);
        break;
      case SystemMediaControlsWebOS::MediaKeyEvent::kUnmute:
        obs.OnMuteStateChanged(false);
        break;
      default:
        NOTREACHED() << " key_event: " << key_event << " Not Handled !!!";
        break;
    }
  }
}

}  // namespace internal

}  // namespace system_media_controls
