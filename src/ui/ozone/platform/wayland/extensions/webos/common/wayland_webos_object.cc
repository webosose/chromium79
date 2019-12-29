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

#include "ui/ozone/platform/wayland/extensions/webos/common/wayland_webos_object.h"

#include <wayland-webos-shell-client-protocol.h>

namespace wl {

const wl_interface* ObjectTraits<wl_shell>::interface = &wl_shell_interface;
void (*ObjectTraits<wl_shell>::deleter)(wl_shell*) = &wl_shell_destroy;

const wl_interface* ObjectTraits<wl_shell_surface>::interface =
    &wl_shell_surface_interface;
void (*ObjectTraits<wl_shell_surface>::deleter)(wl_shell_surface*) =
    &wl_shell_surface_destroy;

const wl_interface* ObjectTraits<wl_webos_shell>::interface =
    &wl_webos_shell_interface;
void (*ObjectTraits<wl_webos_shell>::deleter)(wl_webos_shell*) =
    &wl_webos_shell_destroy;

const wl_interface* ObjectTraits<wl_webos_shell_surface>::interface =
    &wl_webos_shell_surface_interface;
void (*ObjectTraits<wl_webos_shell_surface>::deleter)(wl_webos_shell_surface*) =
    &wl_webos_shell_surface_destroy;

}  // namespace wl
