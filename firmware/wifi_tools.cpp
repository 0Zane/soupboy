#include "include/wifi_tools.h"

#include <WiFi.h>

namespace {

WiFiScanState state = WiFiScanState::Idle;
int networkCount = 0;
uint32_t lastScanAt = 0;

}  // namespace

void wifiToolsBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
}

bool wifiScanStart() {
  if (state == WiFiScanState::Scanning) {
    return false;
  }

  WiFi.scanDelete();
  networkCount = 0;
  const int result = WiFi.scanNetworks(true, false);
  lastScanAt = millis();

  if (result >= 0) {
    networkCount = result;
    state = WiFiScanState::Complete;
  } else if (result == WIFI_SCAN_RUNNING) {
    state = WiFiScanState::Scanning;
  } else {
    state = WiFiScanState::Failed;
  }

  return true;
}

void wifiScanUpdate() {
  if (state != WiFiScanState::Scanning) {
    return;
  }

  const int result = WiFi.scanComplete();
  if (result >= 0) {
    networkCount = result;
    state = WiFiScanState::Complete;
    lastScanAt = millis();
  } else if (result == WIFI_SCAN_FAILED) {
    networkCount = 0;
    state = WiFiScanState::Failed;
    lastScanAt = millis();
  }
}

WiFiScanState wifiScanState() {
  return state;
}

int wifiScanCount() {
  return networkCount;
}

String wifiSSID(uint8_t index) {
  if (index >= networkCount) {
    return "";
  }
  return WiFi.SSID(index);
}

int wifiRSSI(uint8_t index) {
  if (index >= networkCount) {
    return 0;
  }
  return WiFi.RSSI(index);
}

const char *wifiSecurity(uint8_t index) {
  if (index >= networkCount) {
    return "--";
  }
  return WiFi.encryptionType(index) == WIFI_AUTH_OPEN ? "open" : "secure";
}

uint32_t wifiLastScanMs() {
  return lastScanAt;
}
