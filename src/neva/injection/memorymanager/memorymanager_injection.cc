// Copyright 2018-2019 LG Electronics, Inc.
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

#include "injection/memorymanager/memorymanager_injection.h"

#include "base/bind.h"
#include "base/macros.h"
#include "content/browser/system_connector_impl.h"
#include "content/child/child_thread_impl.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "injection/common/public/renderer/injection_base.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "neva/pal_service/public/mojom/memorymanager.mojom.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace injections {

namespace {

// Returns true if |maybe| is both a value, and that value is true.
inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

}  // anonymous namespace

const char MemoryManagerInjectionExtension::kInjectionName[] =
    "v8/memorymanager";

class MemoryManagerInjection : public gin::Wrappable<MemoryManagerInjection>,
                               public pal::mojom::MemoryManagerListener,
                               public InjectionBase {
 public:
  static gin::WrapperInfo kWrapperInfo;
  static const char kGetMemoryStatusMethodName[];
  static const char kOnLevelChangedCallbackName[];

  MemoryManagerInjection();
  void GetMemoryStatus(gin::Arguments* args);

 private:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  void LevelChanged(const std::string& value) override;

  void SubscribeToLevelChanged();
  void OnSubscribeRespond(pal::mojom::MemoryManagerListenerAssociatedRequest request);
  void OnGetMemoryStatusRespond(
      std::unique_ptr<v8::Persistent<v8::Function>> callback,
      const std::string& status);

  mojo::AssociatedBinding<pal::mojom::MemoryManagerListener> listener_binding_;
  mojo::Remote<pal::mojom::MemoryManager> memorymanager_;
};

gin::WrapperInfo MemoryManagerInjection::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

const char MemoryManagerInjection::kGetMemoryStatusMethodName[] =
    "getMemoryStatus";
const char MemoryManagerInjection::kOnLevelChangedCallbackName[] =
    "onlevelchanged";

MemoryManagerInjection::MemoryManagerInjection()
    : listener_binding_(this) {
  service_manager::Connector* connector = content::ChildThreadImpl::current()
                                              ->GetServiceManagerConnection()
                                              ->GetConnector();
  connector->Connect(pal::mojom::kServiceName,
                     memorymanager_.BindNewPipeAndPassReceiver());
  SubscribeToLevelChanged();
}

void MemoryManagerInjection::GetMemoryStatus(gin::Arguments* args) {
  v8::Local<v8::Function> local_func;
  if (!args->GetNext(&local_func)) {
    LOG(ERROR) << __func__ << "(), wrong arguments list";
    return;
  }

  auto callback_ptr =
      std::make_unique<v8::Persistent<v8::Function>>(
          args->isolate(),
          local_func);

  memorymanager_->GetMemoryStatus(base::BindOnce(
      &MemoryManagerInjection::OnGetMemoryStatusRespond,
      base::Unretained(this),
      base::Passed(std::move(callback_ptr))));
}

gin::ObjectTemplateBuilder MemoryManagerInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<MemoryManagerInjection>::
      GetObjectTemplateBuilder(isolate)
          .SetMethod(kGetMemoryStatusMethodName,
                     &MemoryManagerInjection::GetMemoryStatus);
}

void MemoryManagerInjection::LevelChanged(const std::string& json_str) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> callback_key(
      gin::StringToV8(isolate, kOnLevelChangedCallbackName));

  if (!IsTrue(wrapper->Has(wrapper->CreationContext(), callback_key))) {
    LOG(ERROR) << __func__ << "(): no appropriate callback";
    return;
  }

  v8::Local<v8::Value> func_value =
      wrapper->Get(context, callback_key).ToLocalChecked();
  if (!func_value->IsFunction()) {
    LOG(ERROR) << __func__ << "(): callback is not a function";
    return;
  }

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);

  v8::MaybeLocal<v8::Value> maybe_json =
      v8::JSON::Parse(context, gin::StringToV8(isolate, json_str));
  v8::Local<v8::Value> json;
  if (maybe_json.ToLocal(&json)) {
    const int argc = 1;
    v8::Local<v8::Value> argv[] = { json };
    func->Call(context, wrapper, argc, argv);
  } else
    LOG(ERROR) << __func__ << "(): malformed JSON";
}

void MemoryManagerInjection::SubscribeToLevelChanged() {
  memorymanager_->Subscribe(base::BindOnce(
      &MemoryManagerInjection::OnSubscribeRespond,
      base::Unretained(this)));
}

void MemoryManagerInjection::OnSubscribeRespond(
    pal::mojom::MemoryManagerListenerAssociatedRequest request) {
  listener_binding_.Bind(std::move(request));
}

void MemoryManagerInjection::OnGetMemoryStatusRespond(
    std::unique_ptr<v8::Persistent<v8::Function>> callback,
    const std::string& status) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper)) {
    LOG(ERROR) << __func__ << "(): can not get wrapper";
    return;
  }

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Function> local_callback = callback->Get(isolate);

  v8::MaybeLocal<v8::Value> maybe_json =
      v8::JSON::Parse(context, gin::StringToV8(isolate, status));
  v8::Local<v8::Value> json;
  if (maybe_json.ToLocal(&json)) {
      const int argc = 1;
      v8::Local<v8::Value> argv[] = { json };
      local_callback->Call(context, wrapper, argc, argv);
  } else
    LOG(ERROR) << __func__ << "(): malformed JSON";
}

// static
void MemoryManagerInjectionExtension::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> navigator_name = gin::StringToV8(isolate, "navigator");
  v8::Local<v8::Object> navigator;
  if (!gin::Converter<v8::Local<v8::Object>>::FromV8(
          isolate, global->Get(context, navigator_name).ToLocalChecked(),
          &navigator))
    return;

  if (IsTrue(
          navigator->Has(context, gin::StringToV8(isolate, "memorymanager"))))
    return;

  v8::Local<v8::Object> memorymanager;
  if (CreateObject(isolate, navigator).ToLocal(&memorymanager)) {
    memorymanager
        ->Set(context,
              gin::StringToV8(
                  isolate, MemoryManagerInjection::kOnLevelChangedCallbackName),
              v8::Object::New(isolate))
        .Check();
  }
}

void MemoryManagerInjectionExtension::Uninstall(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> navigator_name = gin::StringToV8(isolate, "navigator");
  v8::Local<v8::Object> navigator;
  if (gin::Converter<v8::Local<v8::Object>>::FromV8(
          isolate, global->Get(context, navigator_name).ToLocalChecked(),
          &navigator)) {
    v8::Local<v8::String> memorymanager_name =
        gin::StringToV8(isolate, "memorymanager");
    if (IsTrue(navigator->Has(context, memorymanager_name)))
      navigator->Delete(context, memorymanager_name);
  }
}

// static
v8::MaybeLocal<v8::Object> MemoryManagerInjectionExtension::CreateObject(
    v8::Isolate* isolate, v8::Local<v8::Object> parent) {
  gin::Handle<MemoryManagerInjection> memorymanager =
      gin::CreateHandle(isolate, new MemoryManagerInjection());
  parent
      ->Set(isolate->GetCurrentContext(),
            gin::StringToV8(isolate, "memorymanager"), memorymanager.ToV8())
      .Check();
  return memorymanager->GetWrapper(isolate);
}

}  // namespace injections
