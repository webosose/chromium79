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

#ifndef WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_
#define WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_

#include <map>

#include "net/base/completion_once_callback.h"
#include "net/base/network_delegate_impl.h"
#include "neva/app_runtime/browser/app_runtime_file_access_delegate.h"

namespace webos {

// FIXME(neva): Refactor WebOSNetworkDelegate to WebOSFileAccessDelegate
class WebOSNetworkDelegate
    : public net::NetworkDelegateImpl,
      public neva_app_runtime::AppRuntimeFileAccessDelegate {
 public:
  WebOSNetworkDelegate();

  void append_extra_socket_header(const std::string& key,
                                  const std::string& value) {
    extra_websocket_headers_.insert(std::make_pair(key, value));
  }

  // neva_app_runtime::AppRuntimeFileAccessDelegate implementation
  bool IsAccessAllowed(const base::FilePath& path,
                       int process_id,
                       int route_id,
                       int frame_tree_node_id) const override;

 private:
  void ParsePathsFromSettings(std::vector<std::string>& paths,
                              std::istringstream& stream);

  std::vector<std::string> allowed_target_paths_;
  std::vector<std::string> allowed_trusted_target_paths_;
  std::map<std::string, std::string> extra_websocket_headers_;
  bool allow_all_access_;
};

}  // namespace webos

#endif  // WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_
