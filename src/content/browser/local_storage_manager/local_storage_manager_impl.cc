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

#include "content/browser/local_storage_manager/local_storage_manager_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/data_deleter.h"
#include "content/public/browser/renderer_preferences_util.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"

namespace content {

void LocalStorageManagerImpl::Initialize(const base::FilePath& data_file_path) {
  if (init_status_ != InitializationStatus::kNone)
    return;
  init_status_ = InitializationStatus::kPending;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_runner(
      base::ThreadTaskRunnerHandle::Get());
  scoped_refptr<base::SingleThreadTaskRunner> db_thread_runner =
      base::CreateSingleThreadTaskRunner({content::BrowserThread::IO});
  store_.reset(
      new LocalStorageManagerStore(main_thread_runner, db_thread_runner));
  store_->Initialize(
      data_file_path,
      base::Bind(&LocalStorageManagerImpl::OnStoreInitialized, this));
}

void LocalStorageManagerImpl::OnAppInstalled(const std::string& app_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageManagerImpl not yet initialized";
    return;
  }
  bool inserted = apps_.insert({app_id, true}).second;
  if (inserted) {
    store_->AddApplication(
        ApplicationData{app_id, true},
        base::Bind(&LocalStorageManagerImpl::OnStoreModified, this,
                   StoreModificationOperation::kAddApplication));
  }
  VLOG(1) << "OnAppInstalled appID=" << app_id;
}

void LocalStorageManagerImpl::OnAppRemoved(const std::string& app_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  VLOG(1) << "OnAppRemoved appID=" << app_id;
  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageManagerImpl not yet initialized";
  }
  if (apps_.erase(app_id) == 0) {
    return;
  }
  store_->DeleteApplication(
      app_id, base::Bind(&LocalStorageManagerImpl::OnStoreModified, this,
                         StoreModificationOperation::kDeleteApplication));
  std::set<GURL> origins_to_clear;
  for (auto origin : origins_) {
    AppsSet& apps = origin.second;
    AppsSet::iterator it_app = apps.find(app_id);
    if (it_app != apps.end()) {
      apps.erase(it_app);
      if (apps.size() == 0) {
        VLOG(1) << "Origin=" << origin.first << " to be cleared";
        origins_to_clear.insert(origin.first);
        store_->DeleteOrigin(
            origin.first,
            base::Bind(&LocalStorageManagerImpl::OnStoreModified, this,
                       StoreModificationOperation::kDeleteOrigin));
      }
    }
  }
  if (!origins_to_clear.empty()) {
    VLOG(1) << "Deleting origins begin";
    if (GetDataDeleter())
      GetDataDeleter()->StartDeleting(origins_to_clear,
                                      base::BindOnce(base::DoNothing::Once()));
    for (auto origin : origins_to_clear)
      origins_.erase(origin);
  }
}

base::WeakPtr<LocalStorageManager> LocalStorageManagerImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void LocalStorageManagerImpl::OnAccessOrigin(
    const std::string& app_id,
    const GURL& origin,
    base::OnceCallback<void()> callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!IsInitialized()) {
    LOG(ERROR) << "LocalStorageManagerImpl not yet initialized";
    std::move(callback).Run();
    return;
  }
  if (origin.is_empty() || !origin.is_valid()) {
    LOG(ERROR) << "LocalStorageManagerImpl invalid origin value";
    std::move(callback).Run();
    return;
  }
  GURL origin_actual;
  if (origin.SchemeIsFile()) {
    origin_actual = GURL(std::string("file://").append(app_id));
  } else {
    origin_actual = origin;
  }

  AppToStatusMap::iterator it_app = apps_.find(app_id);
  if (it_app == apps_.end()) {
    VLOG(1) << "OnAccessOrigin: adding application, appID=" << app_id;
    it_app = apps_.insert({app_id, false}).first;
    store_->AddApplication(
        {app_id, false},
        base::Bind(&LocalStorageManagerImpl::OnStoreModified, this,
                   StoreModificationOperation::kAddApplication));
  }

  OriginToAppsMap::iterator it_origin = origins_.find(origin_actual);
  if (it_origin == origins_.end()) {
    VLOG(1) << "OnAccessOrigin: adding origin, origin=" << origin_actual;
    it_origin = origins_.insert({origin_actual, AppsSet()}).first;
    store_->AddOrigin({origin_actual},
                      base::Bind(&LocalStorageManagerImpl::OnStoreModified,
                                 this, StoreModificationOperation::kAddOrigin));
  }
  if (it_origin->second.find(app_id) == it_origin->second.end()) {
    VLOG(1) << "Add appID=" << app_id << " to origin=" << origin_actual;
    it_origin->second.insert(app_id);
    store_->AddAccess({app_id, origin_actual},
                      base::Bind(&LocalStorageManagerImpl::OnStoreModified,
                                 this, StoreModificationOperation::kAddAccess));
    if (it_origin->second.size() == 1 && it_app->second) {
      VLOG(1) << "Clear all data for origin=" << origin_actual;
      GetDataDeleter()->StartDeleting(origin_actual, std::move(callback));
      return;
    }
  }
  std::move(callback).Run();
}

void LocalStorageManagerImpl::OnAccessesLoaded(
    bool success,
    const AccessDataList& access_list) {
  VLOG(1) << "Accesses loaded successfully = " << success;
  if (!success) {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
    return;
  }
  for (auto& item : access_list) {
    OriginToAppsMap::iterator it = origins_.find(item.origin_);
    if (it == origins_.end()) {
      it = origins_.insert({item.origin_, {}}).first;
    }
    it->second.insert(item.app_id_);
  }
  init_status_ = InitializationStatus::kSucceeded;
  OnInitializeSucceeded();
}

void LocalStorageManagerImpl::OnApplicationsLoaded(
    bool success,
    const ApplicationDataList& apps_list) {
  VLOG(1) << "Applications loaded successfully = " << success;
  if (success) {
    store_->GetAccesses(
        base::Bind(&LocalStorageManagerImpl::OnAccessesLoaded, this));
    for (auto& item : apps_list) {
      apps_[item.app_id_] = item.installed_;
    }
  } else {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
  }
}

void LocalStorageManagerImpl::OnStoreModified(
    StoreModificationOperation modification_operation,
    bool success) {
  VLOG(1) << "Store modification=" << static_cast<int>(modification_operation)
          << " completed; Successfully = " << success;
}

void LocalStorageManagerImpl::OnStoreInitialized(bool success) {
  VLOG(1) << "Store initialized success=" << success;
  if (success) {
    store_->GetApplications(
        base::Bind(&LocalStorageManagerImpl::OnApplicationsLoaded, this));
  } else {
    init_status_ = InitializationStatus::kFailed;
    OnInitializeFailed();
  }
}

void LocalStorageManagerImpl::OnInitializeFailed() {}

void LocalStorageManagerImpl::OnInitializeSucceeded() {}

bool LocalStorageManagerImpl::IsInitialized() {
  return init_status_ == InitializationStatus::kSucceeded;
}
}  // namespace content
