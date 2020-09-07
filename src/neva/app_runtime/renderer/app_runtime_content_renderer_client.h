// Copyright (c) 2016-2020 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
#define NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_

#include <memory>

#include "components/watchdog/watchdog.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_thread_observer.h"

#if defined(USE_NEVA_EXTENSIONS)
#include "extensions/shell/renderer/shell_extensions_renderer_client.h"

namespace extensions {
class ExtensionsClient;
class ExtensionsGuestViewContainerDispatcher;
class ShellExtensionsRendererClient;
}  // extensions

namespace neva_app_runtime {

class AppRuntimeExtensionsRendererClient
    : public extensions::ShellExtensionsRendererClient {
 public:
  AppRuntimeExtensionsRendererClient() = default;
  ~AppRuntimeExtensionsRendererClient() override = default;

  void OnExtensionLoaded(const extensions::Extension& extension) override;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeExtensionsRendererClient);
};

}  // neva_app_runtime

#endif

namespace neva_app_runtime {

class AppRuntimeContentRendererClient : public content::ContentRendererClient {
 public:
  AppRuntimeContentRendererClient();
  ~AppRuntimeContentRendererClient() override;

  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderThreadStarted() override;

  bool ShouldSuppressErrorPage(content::RenderFrame* render_frame,
                               const GURL& url) override;

  void PrepareErrorPage(content::RenderFrame* render_frame,
                        const blink::WebURLError& error,
                        const std::string& http_method,
                        std::string* error_html) override;

  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;

#if defined(USE_NEVA_EXTENSIONS)
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  blink::WebPlugin* CreatePluginReplacement(
      content::RenderFrame* render_frame,
      const base::FilePath& plugin_path) override;
  void WillSendRequest(blink::WebLocalFrame* frame,
                       ui::PageTransition transition_type,
                       const blink::WebURL& url,
                       const url::Origin* intiator_origin,
                       GURL* new_url,
                       bool* attach_same_site_cookies) override;
  bool IsExternalPepperPlugin(const std::string& module_name) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const content::WebPluginInfo& info,
      const std::string& mime_type,
      const GURL& original_url) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  extensions::ExtensionsClient* CreateExtensionsClient();
#endif

 private:
  class AppRuntimeRenderThreadObserver;
  void OnNetworkAppear();
  void ArmWatchdog();
  std::unique_ptr<content::RenderThreadObserver> render_thread_observer_;
  std::unique_ptr<watchdog::Watchdog> watchdog_;

#if defined(USE_NEVA_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionsClient> extensions_client_;
  std::unique_ptr<extensions::ShellExtensionsRendererClient>
      extensions_renderer_client_;
  std::unique_ptr<extensions::ExtensionsGuestViewContainerDispatcher>
      guest_view_container_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeContentRendererClient);
#endif
};

}  // namespace neva_app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
