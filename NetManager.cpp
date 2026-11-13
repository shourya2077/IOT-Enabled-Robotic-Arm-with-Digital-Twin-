#include "NetManager.h"
#include "Config.h"

void NetManager::begin() {
  seenRev_ = st_.netRev;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(Cfg::AP_SSID, Cfg::AP_PASS);
  WiFi.setAutoReconnect(true);
}

void NetManager::update() {
  if (seenRev_ != st_.netRev) {                    // credentials edited in the dashboard -> reconnect
    seenRev_ = st_.netRev;
    WiFi.disconnect(false);
    lastTry_ = 0;
  }
  if (!staConfigured() || staConnected()) return;
  // Retry slowly: a failing STA keeps scanning channels, which can stall the access point.
  if (lastTry_ && millis() - lastTry_ < 30000) return;
  lastTry_ = millis();
  WiFi.begin(st_.net.staSsid, st_.net.staPass);
}
