// Copyright (c) 2020 LG Electronics, Inc.
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

#ifndef CONTENT_BROWSER_LOCAL_STORAGE_MANAGER_LOCAL_STORAGE_MANAGER_IMPL_H_
#define CONTENT_BROWSER_LOCAL_STORAGE_MANAGER_LOCAL_STORAGE_MANAGER_IMPL_H_

#include "base/memory/singleton.h"
#include "content/browser/local_storage_manager/local_storage_manager.h"

namespace content {

class LocalStorageManagerImpl final : public LocalStorageManager {
 public:
  static LocalStorageManagerImpl* GetInstance();

  void Initialize(const base::FilePath& data_file_path) override;

  void OnAppInstalled(const std::string& app_id) override;
  void OnAppRemoved(const std::string& app_id) override;
  void OnAccessOrigin(const std::string& app_id,
                      const GURL& origin,
                      base::OnceCallback<void()> callback) override;

  base::WeakPtr<LocalStorageManager> GetWeakPtr() override;

 private:
  enum class InitializationStatus { kNone, kPending, kFailed, kSucceeded };
  void OnAccessesLoaded(bool success, const AccessDataList& access_list);
  void OnApplicationsLoaded(bool success, const ApplicationDataList& apps_list);
  enum class StoreModificationOperation {
    kAddAccess,
    kAddApplication,
    kAddOrigin,
    kDeleteApplication,
    kDeleteOrigin
  };
  void OnStoreModified(StoreModificationOperation modification_operation,
                       bool succeess);
  void OnStoreInitialized(bool success);
  void OnInitializeFailed();
  void OnInitializeSucceeded();
  bool IsInitialized();

  using AppToStatusMap = std::map<std::string, bool>;
  using AppsSet = std::set<std::string>;
  using OriginToAppsMap = std::map<GURL, AppsSet>;

  AppToStatusMap apps_;
  OriginToAppsMap origins_;
  std::unique_ptr<LocalStorageManagerStore> store_;

  InitializationStatus init_status_ = InitializationStatus::kNone;

  base::WeakPtrFactory<LocalStorageManagerImpl> weak_ptr_factory_{this};
  friend struct base::DefaultSingletonTraits<LocalStorageManagerImpl>;
  LocalStorageManagerImpl(){};

  DISALLOW_COPY_AND_ASSIGN(LocalStorageManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOCAL_STORAGE_MANAGER_LOCAL_STORAGE_MANAGER_IMPL_H_
