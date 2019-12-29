// Copyright 2017-2018 LG Electronics, Inc.
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

#ifndef NEVA_WAM_DEMO_WAM_DEMO_SERVICE_H_
#define NEVA_WAM_DEMO_WAM_DEMO_SERVICE_H_

#include "content/public/common/main_function_params.h"
#include "emulator/emulator_data_source.h"
#include "neva/wam_demo/wam_demo_webapp.h"

namespace wam_demo {

class WamDemoService :
    private emulator::EmulatorDataDelegate,
    private WebAppWindowImplObserver,
    private BlinkViewObserver {
 public:
  WamDemoService(const content::MainFunctionParams& parameters);
  virtual ~WamDemoService();

  void Launch(const std::string& appid, const std::string& appurl,
              bool fullscreen, bool frameless);
 private:
  void EmulatorSendData(const std::string& command, const std::string& id);
  void BrowserControlCommandNotify(const std::string& name,
                                   const std::vector<std::string>& args);
  void BrowserControlFunctionNotify(const std::string& name,
                                    const std::vector<std::string>& args,
                                    const std::string& result);
  bool SendCommandWithView(neva_app_runtime::WebViewBase* view,
                           const std::string& cmd);
  void LaunchApp(const std::string& value,
                 const std::string& appid,
                 const std::string& appurl,
                 bool shown);
  void OnPerformanceEvent(neva_app_runtime::WebViewBase* window,
                          const std::string& type);

  // from EmulatorDataDelegate
  void DataUpdated(const std::string& url, const std::string& data) override;

  // from WebAppWindowImplObserver
  void OnWindowClosing(WebAppWindowImpl* window) override;
  void CursorVisibilityChanged(WebAppWindowImpl* window,
                               bool visible) override;

  // from BlinkViewObserver
  void OnDocumentLoadFinished(neva_app_runtime::WebViewBase* view) override;
  void OnLoadFailed(neva_app_runtime::WebViewBase* view) override;
  void OnRenderProcessGone(neva_app_runtime::WebViewBase* view) override;
  void OnRenderProcessCreated(neva_app_runtime::WebViewBase* view) override;
  void OnDidLoadingEnd(neva_app_runtime::WebViewBase* window) override;
  void OnDidFirstPaint(neva_app_runtime::WebViewBase* window) override;
  void OnDidFirstContentfulPaint(neva_app_runtime::WebViewBase* window) override;
  void OnDidFirstImagePaint(neva_app_runtime::WebViewBase* window) override;
  void OnDidFirstMeaningfulPaint(neva_app_runtime::WebViewBase* window) override;
  void OnDidNonFirstMeaningfulPaint(neva_app_runtime::WebViewBase* window) override;
  void OnTitleChanged(neva_app_runtime::WebViewBase* view,
                      const std::string& title) override;

  void OnBrowserControlCommand(
      const std::string& appid,
      const std::string& name,
      const std::vector<std::string>& args) override;

  std::string OnBrowserControlFunction(
      const std::string& appid,
      const std::string& name,
      const std::vector<std::string>& args) override;

  const content::MainFunctionParams parameters_;
  std::vector<WamDemoApplication> appslist_;
  std::string profile_name_;
  static const char wam_demo_app_prefix_[];
};

}  // namespace wam_demo

#endif  // NEVA_WAM_DEMO_WAM_DEMO_SERVICE_H_
