// Copyright 2017-2019 LG Electronics, Inc.
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

#include "neva/wam_demo/wam_demo_webapp.h"

#include "base/strings/string_number_conversions.h"
#include "neva/app_runtime/public/app_runtime_event.h"

namespace wam_demo {

bool BlinkView::policy_ = false;

BlinkView::BlinkView(std::string appid,
                     BlinkViewObserver* observer)
    : observer_(observer)
    , appid_(std::move(appid)) {
}

BlinkView::BlinkView(std::string appid,
                     int width, int height,
                     BlinkViewObserver* observer,
                     neva_app_runtime::WebViewProfile* profile)
    : WebViewBase(width, height, profile)
    , observer_(observer)
    , appid_(std::move(appid)) {
}

BlinkView::~BlinkView() {}

void BlinkView::OnLoadProgressChanged(double progress) {
  progress_ = base::NumberToString((int)std::floor(progress * 100 + 0.5));

  std::string progress_title = title_;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);

  LOG(INFO) << __func__ << "(): " << progress_title;
}

void BlinkView::DidFirstFrameFocused() {
  LOG(INFO) << __func__ << "(): Did frame focused is delivered";
}

void BlinkView::DidLoadingEnd() {
  LOG(INFO) << __func__ << "(): Loading end is delivered";

  if (observer_)
    observer_->OnDidLoadingEnd(this);
}

void BlinkView::DidFirstPaint() {
  LOG(INFO) << __func__ << "(): First paint is delivered";

  if (observer_)
    observer_->OnDidFirstPaint(this);
}

void BlinkView::DidFirstContentfulPaint() {
  LOG(INFO) << __func__ << "(): First contentful paint is delivered";

  if (observer_)
    observer_->OnDidFirstContentfulPaint(this);
}

void BlinkView::DidFirstImagePaint() {
  LOG(INFO) << __func__ << "(): First image paint is delivered";

  if (observer_)
    observer_->OnDidFirstImagePaint(this);
}

void BlinkView::DidFirstMeaningfulPaint() {
  LOG(INFO) << __func__ << "(): First meaningful paint is delivered";

  if (observer_)
    observer_->OnDidFirstMeaningfulPaint(this);
}

void BlinkView::DidNonFirstMeaningfulPaint() {
  LOG(INFO) << __func__ << "(): Non first meaningful paint is delivered";

  if (observer_)
    observer_->OnDidNonFirstMeaningfulPaint(this);
}

void BlinkView::TitleChanged(const std::string& title) {
  title_ = title;
  std::string progress_title = title;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);
}

void BlinkView::NavigationHistoryChanged() {
  LOG(INFO) << __func__
            << "(): Navigation history changed notification is delivered";
}

void BlinkView::Close() {}

bool BlinkView::DecidePolicyForResponse(bool is_main_frame,
                                        int status_code,
                                        const std::string& url,
                                        const std::string& status_text) {
  return policy_;
}

void BlinkView::SetDecidePolicyForResponse() {
  policy_ = true;
}

void BlinkView::SetMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Set media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = true;
}

void BlinkView::ClearMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Clear media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = false;
}

bool BlinkView::AcceptsVideoCapture() {
  LOG(INFO) << __func__ << "(): Accepts video capture is delivered";
  return accepts_video_capture_;
}

bool BlinkView::AcceptsAudioCapture() {
  LOG(INFO) << __func__ << "(): Accepts audio capture is delivered";
  return accepts_audio_capture_;
}

void BlinkView::LoadStarted() {
  LOG(INFO) << __func__ << "(): Load started notification is delivered";
}

void BlinkView::LoadFinished(const std::string& url) {
  LOG(INFO) << __func__
            << "(): Load finished notification is delivered"
            << " for url [" << url << "]";
}

void BlinkView::LoadFailed(const std::string& url,
                           int error_code,
                           const std::string& error_description) {
  LOG(INFO) << __func__
            << "(): Load failed notification is delivered"
            << " for url [" << url << "]";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void BlinkView::LoadAborted(const std::string& url) {
  LOG(INFO) << __func__
            << "(): Load aborted notification is delivered"
            << " for url [" << url << "]";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void BlinkView::LoadStopped() {
  LOG(INFO) << __func__ << "(): Load stopped notification is delivered";
}

void BlinkView::RenderProcessCreated(int pid) {
  if (observer_)
    observer_->OnRenderProcessCreated(this);
}

void BlinkView::RenderProcessGone() {
  if (observer_)
    observer_->OnRenderProcessGone(this);
}

void BlinkView::DocumentLoadFinished() {
  LOG(INFO) << __func__
            << "(): Document load finished notification is delivered";
  if (observer_)
    observer_->OnDocumentLoadFinished(this);
}

void BlinkView::DidStartNavigation(const std::string& url, bool is_main_frame) {
  LOG(INFO) << __func__
            << "(): Did start navigation notification is delivered"
            << " for url [" << url << "]";
}

void BlinkView::DidFinishNavigation(const std::string& url, bool is_main_frame) {
  LOG(INFO) << __func__
            << "(): Did finish navigation notification is delivered"
            << " for url [" << url << "]";
}

void BlinkView::DidHistoryBackOnTopPage() {}

void BlinkView::DidClearWindowObject() {
  LOG(INFO) << __func__
            << "(): Did clear window object notification is delivered";
}

void BlinkView::DidSwapCompositorFrame() {
  LOG(INFO) << __func__
            << "(): Did swap window frame notification is delivered";
}

void BlinkView::DidErrorPageLoadedFromNetErrorHelper() {
  LOG(INFO) << __func__
            << "(): Did error page loaded from net error helper";

}
void BlinkView::RunCommand(const std::string& name,
                           const std::vector<std::string>& arguments) {
  if (observer_)
    observer_->OnBrowserControlCommand(appid_, name, arguments);
}

std::string BlinkView::RunFunction(const std::string& name,
                            const std::vector<std::string>& arguments) {
  return observer_
      ? observer_->OnBrowserControlFunction(appid_, name, arguments)
      : std::string();
}

void WebAppWindowImpl::OnWindowClosing() {
  observer_->OnWindowClosing(this);
}

void WebAppWindowImpl::CursorVisibilityChanged(bool visible) {
  observer_->CursorVisibilityChanged(this, visible);
}

bool WebAppWindowImpl::event(neva_app_runtime::AppRuntimeEvent* e) {
  switch (e->GetType()) {
    case neva_app_runtime::AppRuntimeEvent::Close:
      LOG(INFO) << __func__ << ": Close event is handled";
      break;
    case neva_app_runtime::AppRuntimeEvent::Expose:
      LOG(INFO) << __func__ << ": Expose event is handled";
      break;
    case neva_app_runtime::AppRuntimeEvent::WindowStateChange:
      LOG(INFO) << __func__ << ": WindowStateChange event is handled";
      break;
    case neva_app_runtime::AppRuntimeEvent::WindowStateAboutToChange:
      LOG(INFO) << __func__ << ": WindowStateAboutToChange event is handled";
      break;
    default:
      break;
  }
  return true;
}

WamDemoApplication::WamDemoApplication(const std::string& appid,
                                       const std::string& url,
                                       WebAppWindowImpl* win,
                                       BlinkView* page,
                                       neva_app_runtime::WebViewProfile* profile)
    : appid_(appid),
      url_(url),
      win_(win),
      page_(page),
      profile_(profile),
      creation_time_(base::Time::Now()) {}

WamDemoApplication::WamDemoApplication(const WamDemoApplication& other)
    : appid_(other.appid_),
      url_(other.url_),
      win_(other.win_),
      page_(other.page_),
      profile_(other.profile_),
      creation_time_(other.creation_time_) {}

WamDemoApplication::~WamDemoApplication() {}

}  // namespace wam_demo
