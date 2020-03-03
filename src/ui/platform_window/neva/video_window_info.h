#ifndef UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_
#define UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_

#include <string>

#include "base/unguessable_token.h"

namespace ui {
struct VideoWindowInfo {
  base::UnguessableToken window_id;
  std::string native_window_id;
};

struct VideoWindowParams {
  // if true, overlay procssor will be used for setting video window geometry
  // otherwise the client needs to set geomtery manually.
  bool use_overlay_processor_layout = true;
  // if true, video mute property will be set when video window is invisible
  bool use_video_mute_on_invisible = true;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_NEVA_VIDEO_WINDOW_INFO_H_
