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

#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/neva/base_switches.h"
#include "base/rand_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/system/sys_info.h"
#include "cc/base/switches_neva.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/devtools_manager_delegate.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/app_runtime_file_access_delegate.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_context.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "neva/app_runtime/browser/app_runtime_web_contents_view_delegate_creator.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "neva/app_runtime/public/proxy_settings.h"
#include "neva/app_runtime/webview.h"
#include "neva/pal_service/pal_service_factory.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "services/network/cross_origin_read_blocking.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/service_manager/sandbox/switches.h"
#include "ui/base/ui_base_neva_switches.h"

#if defined(USE_NEVA_EXTENSIONS)
#include "apps/launcher.h"
#include "base/path_service.h"
#include "extensions/browser/guest_view/extensions_guest_view_message_filter.h"
#include "extensions/common/switches.h"
#include "extensions/shell/browser/shell_extension_system.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#endif

#if defined(USE_NEVA_MEDIA)
#include "neva/pal_media_service/pal_media_service_factory.h"
#include "neva/pal_media_service/public/mojom/constants.mojom.h"
#endif

namespace neva_app_runtime {

namespace {
const char kCacheStoreFile[] = "Cache";
const int kDefaultDiskCacheSize = 16 * 1024 * 1024;  // default size is 16MB
}  // namespace

namespace {

bool GetConfiguredValueBySwitchName(const char switch_name[], double* value) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switch_name))
    return false;
  if (!base::StringToDouble(command_line.GetSwitchValueASCII(switch_name),
                            value))
    return false;
  return true;
}

// Skews |value| by +/- |percent|.
int64_t RandomizeByPercent(int64_t value, int percent) {
  double random_percent = (base::RandDouble() - 0.5) * percent * 2;
  return value + (value * (random_percent / 100.0));
}

base::Optional<storage::QuotaSettings> GetConfiguredQuotaSettings(
    const base::FilePath& partition_path) {
  int64_t total = base::SysInfo::AmountOfTotalDiskSpace(partition_path);
  const int kRandomizedPercentage = 10;
  const double kShouldRemainAvailableRatio = 0.1;  // 10%
  const double kMustRemainAvailableRatio = 0.01;   // 1%

  storage::QuotaSettings settings;
  double ratio;
  if (!GetConfiguredValueBySwitchName(kQuotaPoolSizeRatio, &ratio))
    return base::Optional<storage::QuotaSettings>();

  settings.pool_size =
      std::min(RandomizeByPercent(total, kRandomizedPercentage),
               static_cast<int64_t>(total * ratio));

  if (!GetConfiguredValueBySwitchName(kPerHostQuotaRatio, &ratio))
    return base::Optional<storage::QuotaSettings>();

  settings.per_host_quota =
      std::min(RandomizeByPercent(total, kRandomizedPercentage),
               static_cast<int64_t>(settings.pool_size * ratio));
  settings.session_only_per_host_quota = settings.per_host_quota;
  settings.should_remain_available =
      static_cast<int64_t>(total * kShouldRemainAvailableRatio);
  settings.must_remain_available =
      static_cast<int64_t>(total * kMustRemainAvailableRatio);
  settings.refresh_interval = base::TimeDelta::Max();

  return base::make_optional<storage::QuotaSettings>(std::move(settings));
}

#if defined(USE_NEVA_EXTENSIONS)
void LoadAppsFromCommandLine(extensions::ShellExtensionSystem* extension_system,
                             content::BrowserContext* browser_context) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(extensions::switches::kLoadApps)) {
    LOG(ERROR) << "No app specified. Use --" << extensions::switches::kLoadApps
               << " to load and launch an app.";
    return;
  }

  base::CommandLine::StringType path_list =
      command_line->GetSwitchValueNative(extensions::switches::kLoadApps);

  base::StringTokenizerT<base::CommandLine::StringType,
                         base::CommandLine::StringType::const_iterator>
      tokenizer(path_list, FILE_PATH_LITERAL(","));

  const extensions::Extension* launch_app = nullptr;
  while (tokenizer.GetNext()) {
    base::FilePath app_absolute_dir =
        base::MakeAbsoluteFilePath(base::FilePath(tokenizer.token()));

    const extensions::Extension* extension =
        extension_system->LoadApp(app_absolute_dir);
    if (extension && !launch_app)
      launch_app = extension;
  }

  if (launch_app) {
    base::FilePath current_directory;
    base::PathService::Get(base::DIR_CURRENT, &current_directory);
    apps::LaunchPlatformAppWithCommandLine(browser_context, launch_app,
                                           *command_line, current_directory,
                                           extensions::SOURCE_COMMAND_LINE);

    URLPattern pattern(extensions::Extension::kValidHostPermissionSchemes);
    pattern.SetMatchAllURLs(true);
    const_cast<extensions::Extension*>(launch_app)
        ->AddWebExtentPattern(pattern);

  } else {
    LOG(ERROR) << "Could not load any apps.";
  }
  return;
}
#endif

}  // namespace

AppRuntimeContentBrowserClient::AppRuntimeContentBrowserClient(
    net::NetworkDelegate* delegate,
    AppRuntimeQuotaPermissionDelegate* quota_permission_delegate,
    AppRuntimeFileAccessDelegate* file_access_delegate)
    : url_request_context_factory_(new URLRequestContextFactory(delegate)),
      quota_permission_delegate_(quota_permission_delegate),
      file_access_delegate_(file_access_delegate) {}

AppRuntimeContentBrowserClient::~AppRuntimeContentBrowserClient() {}

void AppRuntimeContentBrowserClient::SetBrowserExtraParts(
    AppRuntimeBrowserMainExtraParts* browser_extra_parts) {
  browser_extra_parts_ = browser_extra_parts;
}

std::unique_ptr<content::BrowserMainParts>
AppRuntimeContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  main_parts_ =
      new AppRuntimeBrowserMainParts(url_request_context_factory_.get());

  if (browser_extra_parts_)
    main_parts_->AddParts(browser_extra_parts_);

  return base::WrapUnique(main_parts_);
}

content::WebContentsViewDelegate*
AppRuntimeContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return CreateAppRuntimeWebContentsViewDelegate(web_contents);
}

void AppRuntimeContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    bool is_main_frame_request,
    bool strict_enforcement,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  // HCAP requirements: For SSL Certificate error, follows the policy settings
  if (web_contents && web_contents->GetDelegate()) {
    WebView* webView = static_cast<WebView*>(web_contents->GetDelegate());
    switch (webView->GetSSLCertErrorPolicy()) {
      case SSL_CERT_ERROR_POLICY_IGNORE:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE);
        return;
      case SSL_CERT_ERROR_POLICY_DENY:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
        return;
      default:
        break;
    }
  }

  // A certificate error. The user doesn't really have a context for making the
  // right decision, so block the request hard, without adding info bar that
  // provides possibility to show the insecure content.
  callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
}

void AppRuntimeContentBrowserClient::RunServiceInstance(
    const service_manager::Identity& identity,
    mojo::PendingReceiver<service_manager::mojom::Service>* receiver) {
  const std::string& service_name = identity.name();
  if (service_name == pal::mojom::kServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        pal::CreatePalService(std::move(*receiver)));
#if defined(USE_NEVA_MEDIA)
  } else if (service_name == pal_media::mojom::kServiceName) {
    service_manager::Service::RunAsyncUntilTermination(
        pal_media::CreatePalMediaService(std::move(*receiver)));
#endif
  }
}

bool AppRuntimeContentBrowserClient::ShouldEnableStrictSiteIsolation() {
  // TODO(neva): Temporarily disabled until we support site isolation.
  return false;
}

bool AppRuntimeContentBrowserClient::IsFileAccessAllowedForRequest(
    const base::FilePath& path,
    const base::FilePath& absolute_path,
    const base::FilePath& profile_path,
    const network::ResourceRequest& request) {
  if (!file_access_delegate_)
    return base::CommandLine::ForCurrentProcess()->HasSwitch(kAllowFileAccess);

  if (request.process_id != network::ResourceRequest::kBrowserProcessId)
    return true;
  // If ResourceRequest was created by the browser process, then process_id
  // should be kBrowserProcessId and render_frame_id corresponds to the
  // frame_tree_node_id
  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(request.render_frame_id);
  int process_id = frame_tree_node->current_frame_host()->GetProcess()->GetID();
  int route_id = frame_tree_node->current_frame_host()
                     ->GetRenderViewHost()
                     ->GetRoutingID();

  return file_access_delegate_->IsAccessAllowed(path, process_id, route_id,
                                                request.render_frame_id);
}

bool AppRuntimeContentBrowserClient::ShouldIsolateErrorPage(
    bool /* in_main_frame */) {
  // TODO(neva): Temporarily disabled until we support site isolation.
  return false;
}

void AppRuntimeContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  command_line->AppendSwitch(service_manager::switches::kNoSandbox);

  // Append v8 snapshot path if exists
  auto iter = v8_snapshot_pathes_.find(child_process_id);
  if (iter != v8_snapshot_pathes_.end()) {
    command_line->AppendSwitchPath(switches::kV8SnapshotBlobPath,
                                   base::FilePath(iter->second));
    v8_snapshot_pathes_.erase(iter);
  }

  // Append v8 extra flags if exists
  iter = v8_extra_flags_.find(child_process_id);
  if (iter != v8_extra_flags_.end()) {
    std::string js_flags = iter->second;
    // If already has, append it also
    if (command_line->HasSwitch(switches::kJavaScriptFlags)) {
      js_flags.append(" ");
      js_flags.append(
          command_line->GetSwitchValueASCII(switches::kJavaScriptFlags));
    }
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, js_flags);
    v8_extra_flags_.erase(iter);
  }

  // Append native scroll related flags if native scroll is on by appinfo.json
  auto iter_ns = use_native_scroll_map_.find(child_process_id);
  if (iter_ns != use_native_scroll_map_.end()) {
    bool use_native_scroll = iter_ns->second;
    if (use_native_scroll) {
      // Enables EnableNativeScroll, which is only enabled when there is
      // 'useNativeScroll': true in appinfo.json. If this flag is enabled,
      // Duration of the scroll offset animation is modified.
      if (!command_line->HasSwitch(cc::switches::kEnableWebOSNativeScroll))
        command_line->AppendSwitch(cc::switches::kEnableWebOSNativeScroll);

      // Enables SmoothScrolling, which is mandatory to enable
      // CSSOMSmoothScroll.
      if (!command_line->HasSwitch(switches::kEnableSmoothScrolling))
        command_line->AppendSwitch(switches::kEnableSmoothScrolling);

      // Adds CSSOMSmoothScroll to EnableBlinkFeatures.
      std::string enable_blink_features_flags = "CSSOMSmoothScroll";
      if (command_line->HasSwitch(switches::kEnableBlinkFeatures)) {
        enable_blink_features_flags.append(",");
        enable_blink_features_flags.append(
            command_line->GetSwitchValueASCII(switches::kEnableBlinkFeatures));
      }
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      enable_blink_features_flags);

      // Enables PreferCompositingToLCDText. If this flag is enabled, Compositor
      // thread handles scrolling and disable LCD-text(AntiAliasing) in the
      // scroll area.
      // See PaintLayerScrollableArea.cpp::layerNeedsCompositingScrolling()
      if (!command_line->HasSwitch(switches::kEnablePreferCompositingToLCDText))
        command_line->AppendSwitch(switches::kEnablePreferCompositingToLCDText);
    }

    use_native_scroll_map_.erase(iter_ns);
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseOzoneWaylandVkb))
    command_line->AppendSwitch(switches::kUseOzoneWaylandVkb);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kOzoneWaylandUseXDGShell))
    command_line->AppendSwitch(switches::kOzoneWaylandUseXDGShell);
}

void AppRuntimeContentBrowserClient::SetUseNativeScroll(
    int child_process_id,
    bool use_native_scroll) {
  use_native_scroll_map_.insert(
      std::pair<int, bool>(child_process_id, use_native_scroll));
}

void AppRuntimeContentBrowserClient::AppendExtraWebSocketHeader(
    const std::string& key,
    const std::string& value) {
  if (network_delegate_)
    network_delegate_->SetWebSocketHeader(key, value);
}

content::DevToolsManagerDelegate*
AppRuntimeContentBrowserClient::GetDevToolsManagerDelegate() {
  return new AppRuntimeDevToolsManagerDelegate();
}

void AppRuntimeContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  if (!render_view_host)
    return;

  RenderViewHostDelegate* delegate = render_view_host->GetDelegate();
  if (delegate)
    delegate->OverrideWebkitPrefs(prefs);
}

scoped_refptr<content::QuotaPermissionContext>
AppRuntimeContentBrowserClient::CreateQuotaPermissionContext() {
  return new AppRuntimeQuotaPermissionContext(quota_permission_delegate_);
}

void AppRuntimeContentBrowserClient::GetQuotaSettings(
    content::BrowserContext* context,
    content::StoragePartition* partition,
    storage::OptionalQuotaSettingsCallback callback) {
  base::Optional<storage::QuotaSettings> quota_settings;
  if ((quota_settings = GetConfiguredQuotaSettings(partition->GetPath())) &&
      quota_settings.has_value()) {
    const int64_t kMBytes = 1024 * 1024;
    LOG(INFO) << "QuotaSettings pool_size: "
              << quota_settings->pool_size / kMBytes << "MB"
              << ", shoud_remain_available: "
              << quota_settings->should_remain_available / kMBytes << "MB"
              << ", must_remain_available: "
              << quota_settings->must_remain_available / kMBytes << "MB"
              << ", per_host_quota: "
              << quota_settings->per_host_quota / kMBytes << "MB"
              << ", session_only_per_host_quota: "
              << quota_settings->session_only_per_host_quota / kMBytes << "MB";

    std::move(callback).Run(*quota_settings);
    return;
  }

  storage::GetNominalDynamicSettings(
      partition->GetPath(), context->IsOffTheRecord(),
      storage::GetDefaultDiskInfoHelper(), std::move(callback));
}

content::GeneratedCodeCacheSettings
AppRuntimeContentBrowserClient::GetGeneratedCodeCacheSettings(
    content::BrowserContext* context) {
  return content::GeneratedCodeCacheSettings(true, 0, context->GetPath());
}

void AppRuntimeContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  ContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
      additional_schemes);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFileAPIDirectoriesAndSystem))
    additional_schemes->push_back(url::kFileScheme);
}

std::unique_ptr<content::LoginDelegate>
AppRuntimeContentBrowserClient::CreateLoginDelegate(
    const net::AuthChallengeInfo& auth_info,
    content::WebContents* web_contents,
    const content::GlobalRequestID& request_id,
    bool is_request_for_main_frame,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool first_auth_attempt,
    LoginAuthRequiredCallback auth_required_callback) {
  if (!auth_required_callback.is_null() && !credentials_.Empty()) {
    base::PostTask(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(std::move(auth_required_callback), credentials_));
    return std::make_unique<content::LoginDelegate>();
  }
  return nullptr;
}

void AppRuntimeContentBrowserClient::SetV8SnapshotPath(
    int child_process_id,
    const std::string& path) {
  v8_snapshot_pathes_.insert(
      std::make_pair(child_process_id, path));
}

void AppRuntimeContentBrowserClient::SetV8ExtraFlags(int child_process_id,
                                                     const std::string& flags) {
  v8_extra_flags_.insert(std::make_pair(child_process_id, flags));
}

std::string AppRuntimeContentBrowserClient::GetUserAgent() {
  return neva_app_runtime::GetUserAgent();
}

mojo::Remote<network::mojom::NetworkContext>
AppRuntimeContentBrowserClient::CreateNetworkContext(
    content::BrowserContext* context,
    bool in_memory,
    const base::FilePath& relative_partition_path) {
  mojo::Remote<network::mojom::NetworkContext> network_context;
  network::mojom::NetworkContextParamsPtr context_params =
      network::mojom::NetworkContextParams::New();
  context_params->user_agent = GetUserAgent();
  context_params->accept_language = "en-us,en";

  int disk_cache_size = kDefaultDiskCacheSize;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kDiskCacheSize))
    base::StringToInt(cmd_line->GetSwitchValueASCII(kDiskCacheSize),
                      &disk_cache_size);

  context_params->custom_proxy_config_client_receiver =
      custom_proxy_config_client_.BindNewPipeAndPassReceiver();
  context_params->network_delegate_request =
      mojo::MakeRequest(&network_delegate_);
  context_params->http_cache_max_size = disk_cache_size;
  context_params->http_cache_path = context->GetPath().Append(kCacheStoreFile);
  if (static_cast<const AppRuntimeBrowserContext*>(context)->IsDefault())
    context_params->primary_network_context = true;

  content::GetNetworkService()->CreateNetworkContext(
      network_context.BindNewPipeAndPassReceiver(), std::move(context_params));
  return network_context;
}

void AppRuntimeContentBrowserClient::SetProxyServer(
    const ProxySettings& proxy_settings) {
  if (custom_proxy_config_client_) {
    network::mojom::CustomProxyConfigPtr proxy_config =
        network::mojom::CustomProxyConfig::New();

    credentials_ = net::AuthCredentials();
    if (proxy_settings.enabled) {
      credentials_ =
          net::AuthCredentials(base::UTF8ToUTF16(proxy_settings.username),
                               base::UTF8ToUTF16(proxy_settings.password));
      std::string proxy_string = proxy_settings.ip + ":" + proxy_settings.port;
      net::ProxyConfig::ProxyRules proxy_rules;
      proxy_rules.ParseFromString(proxy_string);

      std::string proxy_bypass_list = proxy_settings.bypass_list;
      // Merge given settings bypass list with one from command line.
      if (base::CommandLine::ForCurrentProcess()->HasSwitch(kProxyBypassList)) {
        std::string cmd_line_proxy_bypass_list =
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                kProxyBypassList);
        if (!proxy_bypass_list.empty())
          proxy_bypass_list += ',';
        proxy_bypass_list += cmd_line_proxy_bypass_list;
      }

      if (!proxy_bypass_list.empty())
        proxy_rules.bypass_rules.ParseFromString(
            proxy_bypass_list,
            net::ProxyBypassRules::ParseFormat::kHostnameSuffixMatching);

      proxy_config->rules = proxy_rules;
    }
    custom_proxy_config_client_->OnCustomProxyConfigUpdated(
        std::move(proxy_config));
  }
}

#if defined(USE_NEVA_EXTENSIONS)
void AppRuntimeContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host,
    service_manager::mojom::ServiceRequest* service_request) {
  using namespace extensions;
  int render_process_id = host->GetID();
  BrowserContext* browser_context =
      main_parts_->GetDefaultBrowserContext()->GetBrowserContext();
  host->AddFilter(
      new ExtensionsGuestViewMessageFilter(render_process_id, browser_context));
}

void AppRuntimeContentBrowserClient::LoadExtensions(
    content::BrowserContext* browser_context) {
  extensions::ShellExtensionSystem* extension_system =
      static_cast<extensions::ShellExtensionSystem*>(
          extensions::ExtensionSystem::Get(browser_context));
  extension_system->FinishInitialization();

  LoadAppsFromCommandLine(extension_system, browser_context);
}
#endif
void AppRuntimeContentBrowserClient::PushCORBDisabledToIOThread(int process_id,
                                                                bool disabled) {
  base::PostTask(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&AppRuntimeContentBrowserClient::SetCORBDisabledOnIOThread,
                     base::Unretained(this), process_id, disabled));
}

void AppRuntimeContentBrowserClient::SetCORBDisabledOnIOThread(int process_id,
                                                               bool disabled) {
  if (!GetNetworkService())
    return;

  if (disabled) {
    GetNetworkService()->AddCorbExceptionForProcess(process_id);
  } else {
    GetNetworkService()->RemoveCorbExceptionForProcess(process_id);
  }
}

}  // namespace neva_app_runtime
