// Copyright 2017-2019 LG Electronics, Inc.
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

#include "neva/pal_media_service/mediaplayer_base_impl.h"
#include "neva/pal_media_service/pal_media_player_factory.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/cdm_context.h"
#include "media/base/media_switches.h"
#include "media/base/pipeline.h"
#include "ui/gfx/geometry/rect_f.h"

namespace pal_media {

static mojom::MediaError convertToMediaError(media::PipelineStatus status) {
  switch (status) {
    case media::PipelineStatus::PIPELINE_OK:
      return mojom::MediaError::kMediaErrorNone;

    case media::PipelineStatus::PIPELINE_ERROR_NETWORK:
      return mojom::MediaError::kMediaErrorInvalidCode;

    case media::PipelineStatus::PIPELINE_ERROR_DECODE:
      return mojom::MediaError::kMediaErrorDecode;

    case media::PipelineStatus::PIPELINE_ERROR_DECRYPT:
    case media::PipelineStatus::PIPELINE_ERROR_ABORT:
    case media::PipelineStatus::PIPELINE_ERROR_INITIALIZATION_FAILED:
    case media::PipelineStatus::PIPELINE_ERROR_COULD_NOT_RENDER:
    case media::PipelineStatus::PIPELINE_ERROR_READ:
    case media::PipelineStatus::PIPELINE_ERROR_INVALID_STATE:
      return mojom::MediaError::kMediaErrorFormat;

    // Demuxer related errors.
    case media::PipelineStatus::DEMUXER_ERROR_COULD_NOT_OPEN:
    case media::PipelineStatus::DEMUXER_ERROR_COULD_NOT_PARSE:
    case media::PipelineStatus::DEMUXER_ERROR_NO_SUPPORTED_STREAMS:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // Decoder related errors.
    case media::PipelineStatus::DECODER_ERROR_NOT_SUPPORTED:
      return mojom::MediaError::kMediaErrorFormat;

    // resource is released by policy action
    case media::PipelineStatus::DECODER_ERROR_RESOURCE_IS_RELEASED:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // ChunkDemuxer related errors.
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_APPEND_FAILED:
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_EOS_STATUS_DECODE_ERROR:
    case media::PipelineStatus::CHUNK_DEMUXER_ERROR_EOS_STATUS_NETWORK_ERROR:
      return mojom::MediaError::kMediaErrorInvalidCode;

    // Audio rendering errors.
    case media::PipelineStatus::AUDIO_RENDERER_ERROR:
      return mojom::MediaError::kMediaErrorInvalidCode;

    default:
      return mojom::MediaError::kMediaErrorInvalidCode;
  }
  return mojom::MediaError::kMediaErrorNone;
}

MediaPlayerBaseImpl::MediaPlayerBaseImpl() {}

MediaPlayerBaseImpl::~MediaPlayerBaseImpl() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MediaPlayerBaseImpl::Subscribe(SubscribeCallback callback) {
  mojom::MediaPlayerListenerAssociatedPtr listener;
  mojom::MediaPlayerListenerAssociatedRequest request =
      mojo::MakeRequest(&(listener));
  listeners_.AddPtr(std::move(listener));
  std::move(callback).Run(std::move(request));
}

void MediaPlayerBaseImpl::OnPlaybackStateChanged(bool playing) {
  DVLOG(1);
  if (playing) {
    listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
      listener->OnMediaPlayerPlay();
    });
  } else {
    listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
      listener->OnMediaPlayerPause();
    });
  }
}

void MediaPlayerBaseImpl::OnStreamEnded() {
  DVLOG(1);
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnPlaybackComplete();
  });
}

void MediaPlayerBaseImpl::OnSeekDone(media::PipelineStatus status,
                                     const base::TimeDelta& current_time) {
  DVLOG(1);
  if (status != media::PIPELINE_OK) {
    listeners_.ForAllPtrs([&status](mojom::MediaPlayerListener* listener) {
      listener->OnMediaError(convertToMediaError(status));
    });
    return;
  }
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnSeekComplete(current_time);
  });
}

void MediaPlayerBaseImpl::OnError(media::PipelineStatus error) {
  DVLOG(1);
  listeners_.ForAllPtrs([&error](mojom::MediaPlayerListener* listener) {
    listener->OnMediaError(convertToMediaError(error));
  });
}

void MediaPlayerBaseImpl::OnBufferingState(
    WebOSMediaClient::BufferingState buffering_state,
    double duration,
    const gfx::Size& natural_video_size) {
  DVLOG(2) << " state:" << buffering_state;

  // TODO(neva): Ensure following states.
  switch (buffering_state) {
    case WebOSMediaClient::kHaveMetadata: {
      listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
        listener->OnMediaMetadataChanged(
            base::TimeDelta::FromSecondsD(duration), natural_video_size.width(),
            natural_video_size.height(), true);
      });
    } break;
    case WebOSMediaClient::kLoadCompleted:
      listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
        listener->OnLoadComplete();
      });
      break;
    case WebOSMediaClient::kPreloadCompleted:
      listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
        listener->OnLoadComplete();
      });
      break;
    case WebOSMediaClient::kPrerollCompleted:
      break;
    case WebOSMediaClient::kWebOSBufferingStart:
      break;
    case WebOSMediaClient::kWebOSBufferingEnd:
      break;
    case WebOSMediaClient::kWebOSNetworkStateLoading:
      break;
    case WebOSMediaClient::kWebOSNetworkStateLoaded:
      break;
  }
}

void MediaPlayerBaseImpl::OnDurationChange() {
  DVLOG(1);
}

void MediaPlayerBaseImpl::OnVideoSizeChange(
    const gfx::Size& natural_video_size) {
  DVLOG(1);
  listeners_.ForAllPtrs(
      [&natural_video_size](mojom::MediaPlayerListener* listener) {
        listener->OnVideoSizeChanged(natural_video_size.width(),
                                     natural_video_size.height());
      });
}

void MediaPlayerBaseImpl::UpdateUMSInfo(const std::string& detail) {
  DVLOG(1);
  if (!detail.empty()) {
    listeners_.ForAllPtrs([&detail](mojom::MediaPlayerListener* listener) {
      listener->OnCustomMessage(
          pal_media::mojom::MediaEventType::kMediaEventUpdateUMSMediaInfo,
          detail);
    });
  }
}

void MediaPlayerBaseImpl::OnAddAudioTrack(
    const std::vector<media::MediaTrackInfo>& audio_track_info) {
  listeners_.ForAllPtrs(
      [&audio_track_info](mojom::MediaPlayerListener* listener) {
        listener->OnAudioTracksUpdated(audio_track_info);
      });
}

void MediaPlayerBaseImpl::OnAddVideoTrack(const std::string& id,
                                          const std::string& kind,
                                          const std::string& language,
                                          bool enabled) {
  NOTIMPLEMENTED();
}

void MediaPlayerBaseImpl::OnAudioFocusChanged() {
  DVLOG(1);
  listeners_.ForAllPtrs([](mojom::MediaPlayerListener* listener) {
    listener->OnAudioFocusChanged();
  });
}

void MediaPlayerBaseImpl::ActiveRegionChanged(const gfx::Rect& active_region) {
  DVLOG(1) << gfx::Rect(active_region).ToString();

  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnActiveRegionChanged(
        gfx::Rect(active_region.x(), active_region.y(), active_region.width(),
                  active_region.height()));
  });
}

void MediaPlayerBaseImpl::OnWaitingForDecryptionKey() {
  NOTIMPLEMENTED();
}

void MediaPlayerBaseImpl::OnEncryptedMediaInitData(
    const std::string& init_data_type,
    const std::vector<uint8_t>& init_data) {
  NOTIMPLEMENTED();
}

void MediaPlayerBaseImpl::OnTimeUpdateTimerFired(
    const base::TimeDelta& current_time) {
  DVLOG(2);
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnTimeUpdate(current_time, base::TimeTicks::Now());
  });
}

void MediaPlayerBaseImpl::OnMediaMetadataChanged(
    const base::TimeDelta& duration,
    int width,
    int height,
    bool success) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnMediaMetadataChanged(duration, width, height, success);
  });
}

void MediaPlayerBaseImpl::OnLoadComplete() {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnLoadComplete();
  });
}

void MediaPlayerBaseImpl::OnCustomMessage(
    const pal_media::mojom::MediaEventType media_event_type,
    const std::string& detail) {
  listeners_.ForAllPtrs([&](mojom::MediaPlayerListener* listener) {
    listener->OnCustomMessage(media_event_type, detail);
  });
}

MediaPlayerProviderImpl::MediaPlayerProviderImpl() {}
MediaPlayerProviderImpl::~MediaPlayerProviderImpl() {}

void MediaPlayerProviderImpl::GetMediaPlayer(
    mojom::MediaPlayerType media_player_type,
    mojom::MediaPlayerRequest request) {
  mojo::MakeStrongBinding(
      PalMediaPlayerFactory::Get()->CreateMediaPlayer(media_player_type),
      std::move(request));
}

}  // namespace pal_media
