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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_

#include "components/local_storage_manager/public/local_storage_manager.h"
#include "content/public/browser/browser_context.h"

namespace neva_app_runtime {

class AppRuntimeContextNetworkDelegate;
class BrowserContextAdapter;
class URLRequestContextFactory;

class AppRuntimeBrowserContext : public content::BrowserContext {
 public:
  static AppRuntimeBrowserContext* Get();
  AppRuntimeBrowserContext(
      const BrowserContextAdapter* adapter,
      URLRequestContextFactory* url_request_context_factory);
  ~AppRuntimeBrowserContext() override;
  base::FilePath GetPath() override;
  bool IsOffTheRecord() override;

  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::StorageNotificationService* GetStorageNotificationService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath&) override;
  content::PermissionControllerDelegate* GetPermissionControllerDelegate()
      override;
  content::ClientHintsControllerDelegate* GetClientHintsControllerDelegate()
      override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate() override;
  content::LocalStorageManager* GetLocalStorageManager();

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);

  void FlushCookieStore();
  void Initialize();

  bool IsDefault() const;

 private:
  class AppRuntimeResourceContext;

  void AppendExtraWebSocketHeaderIO(const std::string& key,
                                    const std::string& value);
  void FlushCookieStoreIO();

  const BrowserContextAdapter* adapter_;
  URLRequestContextFactory* const url_request_context_factory_;
  std::unique_ptr<content::ResourceContext> resource_context_;
#if defined(USE_LOCAL_STORAGE_MANAGER)
  scoped_refptr<content::LocalStorageManager> local_storage_manager_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeBrowserContext);
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_
