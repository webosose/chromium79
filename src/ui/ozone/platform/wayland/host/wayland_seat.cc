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

#include "ui/ozone/platform/wayland/host/wayland_seat.h"

#include "base/bind.h"
#include "base/logging.h"
#include "ui/events/ozone/layout/keyboard_layout_engine_manager.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/ozone/platform/wayland/host/wayland_connection.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor_position.h"
#include "ui/ozone/platform/wayland/host/wayland_keyboard.h"
#include "ui/ozone/platform/wayland/host/wayland_pointer.h"
#include "ui/ozone/platform/wayland/host/wayland_seat_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_touch.h"

namespace ui {

WaylandSeat::WaylandSeat(const uint32_t seat_id, wl_seat* seat)
    : seat_id_(seat_id), seat_(seat) {}

WaylandSeat::~WaylandSeat() = default;

void WaylandSeat::Initialize(WaylandSeatManager* seat_manager) {
  DCHECK(!seat_manager_);
  seat_manager_ = seat_manager;
  static const wl_seat_listener seat_listener = {
      &WaylandSeat::Capabilities,
      &WaylandSeat::Name,
  };
  wl_seat_add_listener(seat_.get(), &seat_listener, this);
}

int WaylandSeat::GetKeyboardModifiers() const {
  int modifiers = 0;
  if (keyboard_)
    modifiers = keyboard_->modifiers();
  return modifiers;
}

// static
void WaylandSeat::Capabilities(void* data,
                               wl_seat* seat,
                               uint32_t capabilities) {
  WaylandSeat* wayland_seat = static_cast<WaylandSeat*>(data);
  DCHECK(wayland_seat);
  DCHECK(wayland_seat->seat_manager_);
  DCHECK(wayland_seat->seat_manager_->connection());
  WaylandConnection* connection = wayland_seat->seat_manager_->connection();
  if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
    if (!wayland_seat->pointer_) {
      wl_pointer* pointer = wl_seat_get_pointer(wayland_seat->seat_.get());
      if (!pointer) {
        LOG(ERROR) << "Failed to get wl_pointer from seat";
        return;
      }
      wayland_seat->pointer_ = std::make_unique<WaylandPointer>(
          pointer, wayland_seat,
          base::BindRepeating(&WaylandConnection::DispatchUiEvent,
                              base::Unretained(connection)));
      wayland_seat->pointer_->set_connection(connection);

      wayland_seat->wayland_cursor_position_ =
          std::make_unique<WaylandCursorPosition>();
    }
  } else if (wayland_seat->pointer_) {
    wayland_seat->pointer_.reset();
    wayland_seat->wayland_cursor_position_.reset();
  }
  if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
    if (!wayland_seat->keyboard_) {
      wl_keyboard* keyboard = wl_seat_get_keyboard(wayland_seat->seat_.get());
      if (!keyboard) {
        LOG(ERROR) << "Failed to get wl_keyboard from seat";
        return;
      }
      wayland_seat->keyboard_ = std::make_unique<WaylandKeyboard>(
          keyboard, KeyboardLayoutEngineManager::GetKeyboardLayoutEngine(),
          base::BindRepeating(&WaylandConnection::DispatchUiEvent,
                              base::Unretained(connection)));
      wayland_seat->keyboard_->set_connection(connection);
    }
  } else if (wayland_seat->keyboard_) {
    wayland_seat->keyboard_.reset();
  }
  if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
    if (!wayland_seat->touch_) {
      wl_touch* touch = wl_seat_get_touch(wayland_seat->seat_.get());
      if (!touch) {
        LOG(ERROR) << "Failed to get wl_touch from seat";
        return;
      }
      wayland_seat->touch_ = std::make_unique<WaylandTouch>(
          touch, base::BindRepeating(&WaylandConnection::DispatchUiEvent,
                                     base::Unretained(connection)));
      wayland_seat->touch_->SetConnection(connection);
    }
  } else if (wayland_seat->touch_) {
    wayland_seat->touch_.reset();
  }
  connection->ScheduleFlush();
}

// static
void WaylandSeat::Name(void* data, wl_seat* seat, const char* name) {}

}  // namespace ui
