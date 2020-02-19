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

#ifndef EXTENSIONS_SHELL_BROWSER_WEBVIEW_CONTROLLER_IMPL_H
#define EXTENSIONS_SHELL_BROWSER_WEBVIEW_CONTROLLER_IMPL_H

#include "content/public/browser/web_contents_binding_set.h"
#include "neva/app_runtime/public/mojom/app_runtime_webview_controller.mojom.h"

namespace content {

class WebContents;

}  // namespace content

namespace extensions {

class WebViewControllerDelegate;

class ExtensionsWebViewControllerImpl
    : public neva_app_runtime::mojom::AppRuntimeWebViewController {
 public:
  ExtensionsWebViewControllerImpl(content::WebContents* web_contents);
  ~ExtensionsWebViewControllerImpl() override;

  void SetDelegate(WebViewControllerDelegate* delegate);

  // using CallFunctionCallback = base::OnceCallback<void(const std::string&)>;
  void CallFunction(const std::string& name,
                    const std::vector<std::string>& args,
                    CallFunctionCallback callback) override;

  void SendCommand(const std::string& name,
                   const std::vector<std::string>& args) override;

 private:
  content::WebContentsFrameBindingSet<
      neva_app_runtime::mojom::AppRuntimeWebViewController>
      bindings_;
  WebViewControllerDelegate* webview_controller_delegate_ = nullptr;
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_WEBVIEW_CONTROLLER_IMPL_H
