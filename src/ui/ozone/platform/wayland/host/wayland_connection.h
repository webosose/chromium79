// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/message_loop/message_pump_for_ui.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/buffer_types.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"
#include "ui/ozone/platform/wayland/host/gtk_primary_selection_device.h"
#include "ui/ozone/platform/wayland/host/gtk_primary_selection_device_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_clipboard.h"
#include "ui/ozone/platform/wayland/host/wayland_cursor_position.h"
#include "ui/ozone/platform/wayland/host/wayland_data_device.h"
#include "ui/ozone/platform/wayland/host/wayland_data_device_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_data_source.h"
#include "ui/ozone/platform/wayland/host/wayland_output.h"
#include "ui/ozone/platform/wayland/host/wayland_pointer.h"
#include "ui/ozone/platform/wayland/host/wayland_seat.h"
#include "ui/ozone/platform/wayland/host/wayland_seat_manager.h"
#include "ui/ozone/platform/wayland/host/wayland_window_manager.h"

namespace ui {

class WaylandBufferManagerHost;
class WaylandExtension;
class WaylandOutputManager;
class WaylandWindow;
class WaylandDrm;
class WaylandZwpLinuxDmabuf;
class WaylandShm;
class WaylandSeatManager;

class WaylandConnection : public PlatformEventSource,
                          public base::MessagePumpForUI::FdWatcher {
 public:
  WaylandConnection();
  ~WaylandConnection() override;

  bool Initialize();
  bool StartProcessingEvents();

  // Schedules a flush of the Wayland connection.
  void ScheduleFlush();

  wl_display* display() const { return display_.get(); }
  wl_compositor* compositor() const { return compositor_.get(); }
  wl_subcompositor* subcompositor() const { return subcompositor_.get(); }
  xdg_shell* shell() const { return shell_.get(); }
  zxdg_shell_v6* shell_v6() const { return shell_v6_.get(); }
  wl_seat* seat() const {
    if (wayland_seat_manager_ && wayland_seat_manager_->GetFirstSeat())
      return wayland_seat_manager_->GetFirstSeat()->seat();
    return nullptr;
  }
  wl_data_device* data_device() const { return data_device_->data_device(); }
  gtk_primary_selection_device* primary_selection_device() const {
    return primary_selection_device_->data_device();
  }
  wp_presentation* presentation() const { return presentation_.get(); }
  zwp_text_input_manager_v1* text_input_manager_v1() const {
    return text_input_manager_v1_.get();
  }

  void set_serial(uint32_t serial) { serial_ = serial; }
  uint32_t serial() const { return serial_; }

  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);

  // Returns the current pointer (from the first seat), which may be null.
  WaylandPointer* pointer() const {
    if (wayland_seat_manager_ && wayland_seat_manager_->GetFirstSeat())
      return wayland_seat_manager_->GetFirstSeat()->pointer();
    return nullptr;
  }

  // Returns the current touch, which may be null.
  WaylandTouch* touch() const {
    if (wayland_seat_manager_ && wayland_seat_manager_->GetFirstSeat())
      return wayland_seat_manager_->GetFirstSeat()->touch();
    return nullptr;
  }

  WaylandClipboard* clipboard() const { return clipboard_.get(); }

  WaylandDataSource* drag_data_source() const {
    return dragdrop_data_source_.get();
  }

  WaylandOutputManager* wayland_output_manager() const {
    return wayland_output_manager_.get();
  }

  WaylandSeatManager* wayland_seat_manager() const {
    return wayland_seat_manager_.get();
  }

  // Returns the cursor position (from the first seat), which may be null.
  WaylandCursorPosition* wayland_cursor_position() const {
    if (wayland_seat_manager_ && wayland_seat_manager_->GetFirstSeat())
      return wayland_seat_manager_->GetFirstSeat()->wayland_cursor_position();
    return nullptr;
  }

  WaylandBufferManagerHost* buffer_manager_host() const {
    return buffer_manager_host_.get();
  }

  WaylandExtension* extension() { return extension_.get(); }

  WaylandZwpLinuxDmabuf* zwp_dmabuf() const { return zwp_dmabuf_.get(); }

  WaylandDrm* drm() const { return drm_.get(); }

  WaylandShm* shm() const { return shm_.get(); }

  WaylandWindowManager* wayland_window_manager() {
    return &wayland_window_manager_;
  }

  // Starts drag with |data| to be delivered, |operation| supported by the
  // source side initiated the dragging.
  void StartDrag(const ui::OSExchangeData& data, int operation);
  // Finishes drag and drop session. It happens when WaylandDataSource gets
  // 'OnDnDFinished' or 'OnCancel', which means the drop is performed or
  // canceled on others.
  void FinishDragSession(uint32_t dnd_action, WaylandWindow* source_window);
  // Delivers the data owned by Chromium which initiates drag-and-drop. |buffer|
  // is an output parameter and it should be filled with the data corresponding
  // to mime_type.
  void DeliverDragData(const std::string& mime_type, std::string* buffer);
  // Requests the data to the platform when Chromium gets drag-and-drop started
  // by others. Once reading the data from platform is done, |callback| should
  // be called with the data.
  void RequestDragData(
      const std::string& mime_type,
      base::OnceCallback<void(const std::vector<uint8_t>&)> callback);

  // Returns true when dragging is entered or started.
  bool IsDragInProgress();

  // Resets flags and keyboard modifiers.
  //
  // This method is specially handy for cases when the WaylandPointer state is
  // modified by a POINTER_DOWN event, but the respective POINTER_UP event is
  // not delivered.
  void ResetPointerFlags();

 private:
  // WaylandInputMethodContextFactory needs access to DispatchUiEvent
  friend class WaylandInputMethodContextFactory;
  // WaylandSeat needs access to DispatchUiEvent
  friend class WaylandSeat;

  void Flush();
  void DispatchUiEvent(Event* event);

  // PlatformEventSource
  void OnDispatcherListChanged() override;

  // base::MessagePumpForUI::FdWatcher
  void OnFileCanReadWithoutBlocking(int fd) override;
  void OnFileCanWriteWithoutBlocking(int fd) override;

  // Make sure data device is properly initialized
  void EnsureDataDevice();

  bool BeginWatchingFd(base::WatchableIOMessagePumpPosix::Mode mode);
  void MaybePrepareReadQueue();

  // wl_registry_listener
  static void Global(void* data,
                     wl_registry* registry,
                     uint32_t name,
                     const char* interface,
                     uint32_t version);
  static void GlobalRemove(void* data, wl_registry* registry, uint32_t name);

  // zxdg_shell_v6_listener
  static void PingV6(void* data, zxdg_shell_v6* zxdg_shell_v6, uint32_t serial);

  // xdg_shell_listener
  static void Ping(void* data, xdg_shell* shell, uint32_t serial);

  wl::Object<wl_display> display_;
  wl::Object<wl_registry> registry_;
  wl::Object<wl_compositor> compositor_;
  wl::Object<wl_subcompositor> subcompositor_;
  wl::Object<xdg_shell> shell_;
  wl::Object<zxdg_shell_v6> shell_v6_;
  wl::Object<wp_presentation> presentation_;
  wl::Object<zwp_text_input_manager_v1> text_input_manager_v1_;

  std::unique_ptr<WaylandDataDeviceManager> data_device_manager_;
  std::unique_ptr<WaylandDataDevice> data_device_;
  std::unique_ptr<WaylandClipboard> clipboard_;
  std::unique_ptr<WaylandDataSource> dragdrop_data_source_;
  std::unique_ptr<WaylandOutputManager> wayland_output_manager_;
  std::unique_ptr<WaylandZwpLinuxDmabuf> zwp_dmabuf_;
  std::unique_ptr<WaylandDrm> drm_;
  std::unique_ptr<WaylandShm> shm_;
  std::unique_ptr<WaylandBufferManagerHost> buffer_manager_host_;
  std::unique_ptr<WaylandExtension> extension_;
  std::unique_ptr<WaylandSeatManager> wayland_seat_manager_;

  std::unique_ptr<GtkPrimarySelectionDeviceManager>
      primary_selection_device_manager_;
  std::unique_ptr<GtkPrimarySelectionDevice> primary_selection_device_;

  // Manages Wayland windows.
  WaylandWindowManager wayland_window_manager_;

  bool scheduled_flush_ = false;
  bool watching_ = false;
  bool prepared_ = false;
  base::MessagePumpForUI::FdWatchController controller_;

  uint32_t serial_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WaylandConnection);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_CONNECTION_H_
