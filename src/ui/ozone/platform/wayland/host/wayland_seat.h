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

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SEAT_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SEAT_H_

#include "base/macros.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"

namespace ui {

class WaylandSeatManager;
class WaylandKeyboard;
class WaylandPointer;
class WaylandTouch;
class WaylandCursorPosition;

// A Seat wrapper implementation for Wayland.
class WaylandSeat {
 public:
  explicit WaylandSeat(const uint32_t seat_id, wl_seat* seat);
  virtual ~WaylandSeat();

  void Initialize(WaylandSeatManager* seat_manager);

  uint32_t seat_id() const { return seat_id_; }
  wl_seat* seat() const { return seat_.get(); }
  // Returns the current pointer, which may be null.
  WaylandPointer* pointer() const { return pointer_.get(); }
  // Returns the current touch, which may be null.
  WaylandTouch* touch() const { return touch_.get(); }
  // Returns the cursor position, which may be null.
  WaylandCursorPosition* wayland_cursor_position() const {
    return wayland_cursor_position_.get();
  }

  int GetKeyboardModifiers() const;

 private:
  // wl_seat_listener
  static void Capabilities(void* data, wl_seat* seat, uint32_t capabilities);
  static void Name(void* data, wl_seat* seat, const char* name);

  WaylandSeatManager* seat_manager_ = nullptr;

  const uint32_t seat_id_ = 0;
  wl::Object<wl_seat> seat_;

  std::unique_ptr<WaylandKeyboard> keyboard_;
  std::unique_ptr<WaylandPointer> pointer_;
  std::unique_ptr<WaylandTouch> touch_;
  std::unique_ptr<WaylandCursorPosition> wayland_cursor_position_;

  DISALLOW_COPY_AND_ASSIGN(WaylandSeat);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_SEAT_H_
