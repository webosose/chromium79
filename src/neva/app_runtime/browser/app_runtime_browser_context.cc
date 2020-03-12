// Copyright 2016-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/browser/app_runtime_browser_context.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task/post_task.h"
#include "browser/app_runtime_browser_context_adapter.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "net/base/http_user_agent_settings.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace neva_app_runtime {

AppRuntimeBrowserContext::AppRuntimeBrowserContext(
    const BrowserContextAdapter* adapter,
    URLRequestContextFactory* url_request_context_factory)
    : adapter_(adapter),
      url_request_context_factory_(url_request_context_factory),
      resource_context_(new content::ResourceContext()) {
  BrowserContext::Initialize(this, GetPath());
#if defined(USE_LOCAL_STORAGE_MANAGER)
  local_storage_manager_ = content::LocalStorageManager::Create().release();
#endif
}

AppRuntimeBrowserContext::~AppRuntimeBrowserContext() {}

base::FilePath AppRuntimeBrowserContext::GetPath() {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kUserDataDir)) {
    base::FilePath path = cmd_line->GetSwitchValuePath(kUserDataDir);
    return path.Append(adapter_->GetStorageName());
  }

  return base::FilePath();
}

bool AppRuntimeBrowserContext::IsOffTheRecord() {
  return false;
}

content::ResourceContext* AppRuntimeBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate*
AppRuntimeBrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

content::BrowserPluginGuestManager*
AppRuntimeBrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy*
AppRuntimeBrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService*
AppRuntimeBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::StorageNotificationService*
AppRuntimeBrowserContext::GetStorageNotificationService() {
  return nullptr;
}

content::SSLHostStateDelegate*
AppRuntimeBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

std::unique_ptr<content::ZoomLevelDelegate>
AppRuntimeBrowserContext::CreateZoomLevelDelegate(const base::FilePath&) {
  return nullptr;
}

content::PermissionControllerDelegate*
AppRuntimeBrowserContext::GetPermissionControllerDelegate() {
  return nullptr;
}

content::BackgroundFetchDelegate* AppRuntimeBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
AppRuntimeBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
AppRuntimeBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

void AppRuntimeBrowserContext::Initialize() {
#if defined(USE_LOCAL_STORAGE_MANAGER)
  local_storage_manager_->Initialize(GetPath());
#endif
}

content::LocalStorageManager*
AppRuntimeBrowserContext::GetLocalStorageManager() {
#if defined(USE_LOCAL_STORAGE_MANAGER)
  return local_storage_manager_.get();
#else
  return nullptr;
#endif
}

void AppRuntimeBrowserContext::FlushCookieStore() {
  base::PostTask(FROM_HERE, {content::BrowserThread::IO},
                 base::Bind(&AppRuntimeBrowserContext::FlushCookieStoreIO,
                            base::Unretained(this)));
}

void AppRuntimeBrowserContext::FlushCookieStoreIO() {
  content::BrowserContext::GetDefaultStoragePartition(this)
      ->GetCookieManagerForBrowserProcess()
      ->FlushCookieStore(
          network::mojom::CookieManager::FlushCookieStoreCallback());
}

content::ClientHintsControllerDelegate*
AppRuntimeBrowserContext::GetClientHintsControllerDelegate() {
  return nullptr;
}

bool AppRuntimeBrowserContext::IsDefault() const {
  return adapter_->IsDefault();
}

}  // namespace neva_app_runtime
