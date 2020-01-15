#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_

namespace blink {

class WebSettingsNeva {
 public:
  virtual void SetKeepAliveWebApp(bool) = 0;
  virtual bool KeepAliveWebApp() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_
