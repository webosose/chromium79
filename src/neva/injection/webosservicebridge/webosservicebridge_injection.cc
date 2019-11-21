// Copyright 2019 LG Electronics, Inc.
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

#include "neva/injection/webosservicebridge/webosservicebridge_injection.h"

#include "base/rand_util.h"
#include "content/browser/system_connector_impl.h"
#include "content/child/child_thread_impl.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_frame.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "neva/pal_service/public/mojom/system_servicebridge.mojom.h"
#include "neva/injection/common/gin/function_template_neva.h"
#include "neva/injection/grit/injection_resources.h"
#include "neva/injection/webosservicebridge/webosservicebridge_properties.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

#include <set>
#include <string>

namespace {

const char kCallMethodName[] = "call";
const char kCancelMethodName[] = "cancel";
const char kOnServiceCallbackName[] = "onservicecallback";
const char kWebOSServiceBridge[] = "WebOSServiceBridge";
const char kWebOSSystemGetIdentifierJS[] = "webOSSystem.getIdentifier()";
const char kWebOSSystemOnCloseNotifyJS[] =
    "webOSSystem.onCloseNotify(\"didRunOnCloseCallback\")";
const char kMethodInvocationAsConstructorOnly[] =
    "WebOSServiceBridge function must be invoked as a constructor only";

}  // anonymous namespace

namespace injections {

class WebOSServiceBridgeInjection
    : public gin::Wrappable<WebOSServiceBridgeInjection>
    , public pal::mojom::SystemServiceBridgeClient {
 public:
  static gin::WrapperInfo kWrapperInfo;

  WebOSServiceBridgeInjection(std::string appid);
  ~WebOSServiceBridgeInjection() override;

  // To handle luna call in webOSSystem.onclose callback
  static std::set<WebOSServiceBridgeInjection*> waiting_responses_;
  static bool is_closing_;

 private:
  void Call(gin::Arguments* args);
  void DoCall(std::string uri, std::string payload);
  void Cancel();
  void DoNothing();

  bool IsWebOSSystemLoaded();
  void CloseNotify();

  void OnConnect(
      pal::mojom::SystemServiceBridgeClientAssociatedRequest request);
  void CallJSHandler(const std::string& body);
  void Response(pal::mojom::ResponseStatus status,
                const std::string& payload) override;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  std::string identifier_;
  mojo::AssociatedBinding<pal::mojom::SystemServiceBridgeClient>
      client_binding_;
  pal::mojom::SystemServiceBridgePtr system_bridge_;
};

gin::WrapperInfo WebOSServiceBridgeInjection::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

bool WebOSServiceBridgeInjection::is_closing_ = false;
std::set<WebOSServiceBridgeInjection*>
    WebOSServiceBridgeInjection::waiting_responses_;

WebOSServiceBridgeInjection::WebOSServiceBridgeInjection(std::string appid)
    : identifier_(std::move(appid))
    , client_binding_(this) {
  mojo::Remote<pal::mojom::SystemServiceBridgeProvider> provider;
  service_manager::Connector* connector = content::ChildThreadImpl::current()
                                              ->GetServiceManagerConnection()
                                              ->GetConnector();
  connector->Connect(pal::mojom::kServiceName,
                     provider.BindNewPipeAndPassReceiver());

  provider->GetSystemServiceBridge(mojo::MakeRequest(&system_bridge_));
  system_bridge_->Connect(
      identifier_,
      base::BindRepeating(
          &WebOSServiceBridgeInjection::OnConnect,
          base::Unretained(this)));
}

WebOSServiceBridgeInjection::~WebOSServiceBridgeInjection() {
  Cancel();
}

void WebOSServiceBridgeInjection::Call(gin::Arguments* args) {
  std::string uri;
  std::string payload;
  if (args->GetNext(&uri) && args->GetNext(&payload))
    DoCall(std::move(uri), std::move(payload));
  else
    DoCall(std::string(""), std::string(""));
}

void WebOSServiceBridgeInjection::DoCall(std::string uri, std::string payload) {
  if (identifier_.empty())
    return;

  system_bridge_->Call(std::move(uri), std::move(payload));
  if (WebOSServiceBridgeInjection::is_closing_) {
    VLOG(1) << "WebOSServiceBridge [Call][" << identifier_ << "]"
            << " uri: " << uri << ", payload:" << payload << " during closing";
    waiting_responses_.insert(this);
  }
}

void WebOSServiceBridgeInjection::Cancel() {
  if (identifier_.empty())
    return;

  system_bridge_->Cancel();
  waiting_responses_.erase(this);
}

void WebOSServiceBridgeInjection::DoNothing() {
}

void WebOSServiceBridgeInjection::OnConnect(
    pal::mojom::SystemServiceBridgeClientAssociatedRequest request) {
  client_binding_.Bind(std::move(request));
}

void WebOSServiceBridgeInjection::CallJSHandler(const std::string& body) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);
  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return;

  auto context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> callback_key(
      gin::StringToV8(isolate, kOnServiceCallbackName));

  v8::Local<v8::Value> func_value =
      wrapper->Get(context, callback_key).ToLocalChecked();
  if (func_value->IsNullOrUndefined() || !func_value->IsFunction())
    return;

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);

  const int argc = 1;
  v8::Local<v8::Value> argv[] = { gin::StringToV8(isolate, body) };
  func->Call(context, wrapper, argc, argv);
}

void WebOSServiceBridgeInjection::Response(pal::mojom::ResponseStatus status,
                                           const std::string& body) {
  if (status == pal::mojom::ResponseStatus::kSuccess)
    CallJSHandler(body);

  if (!WebOSServiceBridgeInjection::is_closing_)
    return;

  VLOG(1) << "WebOSServiceBridge [Response][" << identifier_ << "]"
          << " body:" << body;
  waiting_responses_.erase(this);

  if (waiting_responses_.empty())
    CloseNotify();
}

gin::ObjectTemplateBuilder
    WebOSServiceBridgeInjection::GetObjectTemplateBuilder(
        v8::Isolate* isolate) {
  return gin::Wrappable<WebOSServiceBridgeInjection>::
      GetObjectTemplateBuilder(isolate)
          .SetMethod(kCallMethodName, &WebOSServiceBridgeInjection::Call)
          .SetMethod(kCancelMethodName, &WebOSServiceBridgeInjection::Cancel)
          .SetMethod(kOnServiceCallbackName,
                     &WebOSServiceBridgeInjection::DoNothing);
}

bool WebOSServiceBridgeInjection::IsWebOSSystemLoaded() {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);

  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return false;

  auto context = wrapper->CreationContext();
  if (context.IsEmpty())
    return false;

  v8::Local<v8::Object> global = context->Global();
  v8::Maybe<bool> success =
      global->Has(context, gin::StringToV8(isolate, "webOSSystem"));
  return success.IsJust() && success.FromJust();
}

void WebOSServiceBridgeInjection::CloseNotify() {
  if (!IsWebOSSystemLoaded())
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::MaybeLocal<v8::Object> maybe_wrapper = GetWrapper(isolate);
  v8::Local<v8::Object> wrapper;
  if (!maybe_wrapper.ToLocal(&wrapper))
    return;

  auto context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> source =
      gin::StringToV8(isolate, kWebOSSystemOnCloseNotifyJS);
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source);
  if (!script.IsEmpty())
    script.ToLocalChecked()->Run(context);
}

// WebOSServiceBridgeInjectionExtension

const char WebOSServiceBridgeInjectionExtension::kInjectionName[] =
    "v8/webosservicebridge";
const char WebOSServiceBridgeInjectionExtension::kObsoleteName[] =
    "v8/palmservicebridge";
std::unique_ptr<WebOSServiceBridgeProperties>
    WebOSServiceBridgeInjectionExtension::properties_;


void WebOSServiceBridgeInjectionExtension::Install(
    blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  v8::Local<v8::FunctionTemplate> templ = gin::CreateConstructorTemplate(
      isolate, base::Bind(
          &WebOSServiceBridgeInjectionExtension::
              WebOSServiceBridgeConstructorCallback));

  global
      ->Set(context, gin::StringToSymbol(isolate, kWebOSServiceBridge),
            templ->GetFunction(context).ToLocalChecked())
      .Check();

  const std::string extra_objects_js(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBOSSERVICEBRIDGE_INJECTION_JS));

  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      context, gin::StringToV8(isolate, extra_objects_js.c_str()));
  if (script.ToLocal(&local_script))
    local_script->Run(context);

  properties_ = std::make_unique<WebOSServiceBridgeProperties>(frame);
}

// static
void WebOSServiceBridgeInjectionExtension::Uninstall(
    blink::WebLocalFrame* frame) {
  const std::string extra_objects_js(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBOSSERVICEBRIDGE_ROLLBACK_JS));

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
      context, gin::StringToV8(isolate, extra_objects_js.c_str()));

  if (script.ToLocal(&local_script))
    local_script->Run(context);
  properties_.reset();
}

// static
bool WebOSServiceBridgeInjectionExtension::HasWaitingRequests() {
  return !WebOSServiceBridgeInjection::waiting_responses_.empty();
}

// static
bool WebOSServiceBridgeInjectionExtension::IsClosing() {
  return WebOSServiceBridgeInjection::is_closing_;
}

// static
void WebOSServiceBridgeInjectionExtension::SetAppInClosing(bool closing) {
  WebOSServiceBridgeInjection::is_closing_ = closing;
}

// static
void WebOSServiceBridgeInjectionExtension::
    WebOSServiceBridgeConstructorCallback(gin::Arguments* args) {
  if (!args->IsConstructCall()) {
    args->isolate()->ThrowException(v8::Exception::Error(
        gin::StringToV8(args->isolate(), kMethodInvocationAsConstructorOnly)));
    return;
  }

  if (!properties_)
    return;

  std::string appid = properties_->GetIdentifier();

  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);
  gin::Handle<injections::WebOSServiceBridgeInjection> wrapper =
      gin::CreateHandle(
          isolate,
          new injections::WebOSServiceBridgeInjection(std::move(appid)));
  if (!wrapper.IsEmpty())
    args->Return(wrapper.ToV8());
}

}  // namespace injections
