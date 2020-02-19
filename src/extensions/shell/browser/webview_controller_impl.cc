// Copyright 2020 LG Electronics, Inc.
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

#include "extensions/shell/browser/webview_controller_impl.h"

#include "extensions/shell/webview_controller_delegate.h"

namespace extensions {

ExtensionsWebViewControllerImpl::ExtensionsWebViewControllerImpl(
    content::WebContents* web_contents)
    : bindings_(web_contents, this) {}

ExtensionsWebViewControllerImpl::~ExtensionsWebViewControllerImpl() {}

void ExtensionsWebViewControllerImpl::SetDelegate(
    WebViewControllerDelegate* delegate) {
  webview_controller_delegate_ = delegate;
}

void ExtensionsWebViewControllerImpl::CallFunction(
    const std::string& name,
    const std::vector<std::string>& args,
    CallFunctionCallback callback) {
  std::string result;
  if (webview_controller_delegate_)
    result = webview_controller_delegate_->RunFunction(name, args);
  std::move(callback).Run(std::move(result));
}

void ExtensionsWebViewControllerImpl::SendCommand(
    const std::string& name,
    const std::vector<std::string>& args) {
  if (webview_controller_delegate_)
    webview_controller_delegate_->RunCommand(name, args);
}

}  // namespace extensions
