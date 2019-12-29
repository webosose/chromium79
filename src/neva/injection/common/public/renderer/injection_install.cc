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

#include "injection/common/public/renderer/injection_install.h"

#include "base/logging.h"

#if defined(OS_WEBOS)
#include "neva/injection/webosservicebridge/webosservicebridge_injection.h"
#include "neva/injection/webossystem/webossystem_injection.h"
#endif

#if defined(ENABLE_SAMPLE_WEBAPI)
#include "injection/sample/sample_injection.h"
#endif

#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "injection/browser_control/browser_control_injection.h"
#endif

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
#include "injection/memorymanager/memorymanager_injection.h"
#endif

namespace injections {

bool GetInjectionInstallAPI(const std::string& name, InstallAPI* api) {
  DCHECK(api != nullptr);
#if defined(OS_WEBOS)
  if ((name == WebOSSystemInjectionExtension::kInjectionName) ||
      (name == WebOSSystemInjectionExtension::kObsoleteName)) {
    api->install_func = WebOSSystemInjectionExtension::Install;
    api->uninstall_func = WebOSSystemInjectionExtension::Uninstall;
    return true;
  }

  if ((name == WebOSServiceBridgeInjectionExtension::kInjectionName) ||
      (name == WebOSServiceBridgeInjectionExtension::kObsoleteName)) {
    api->install_func = WebOSServiceBridgeInjectionExtension::Install;
    api->uninstall_func = WebOSServiceBridgeInjectionExtension::Uninstall;
    return true;
  }
#endif
#if defined(ENABLE_SAMPLE_WEBAPI)
  if (name == SampleInjectionExtension::kInjectionName) {
    api->install_func = SampleInjectionExtension::Install;
    api->uninstall_func = SampleInjectionExtension::Uninstall;
    return true;
  }
#endif
#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
  if (name == MemoryManagerInjectionExtension::kInjectionName) {
    api->install_func = MemoryManagerInjectionExtension::Install;
    api->uninstall_func = MemoryManagerInjectionExtension::Uninstall;
    return true;
  }
#endif
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  if (name == BrowserControlInjectionExtension::kInjectionName) {
    api->install_func = BrowserControlInjectionExtension::Install;
    api->uninstall_func = BrowserControlInjectionExtension::Uninstall;
    return true;
  }
#endif
  return false;
}

}  // namespace injections
