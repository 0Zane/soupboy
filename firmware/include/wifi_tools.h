#pragma once

#include <Arduino.h>

enum class WiFiScanState : uint8_t {
  Idle,
  Scanning,
  Complete,
  Failed,
};

void wifiToolsBegin();
bool wifiScanStart();
void wifiScanUpdate();
WiFiScanState wifiScanState();
int wifiScanCount();
String wifiSSID(uint8_t index);
int wifiRSSI(uint8_t index);
const char *wifiSecurity(uint8_t index);
uint32_t wifiLastScanMs();
