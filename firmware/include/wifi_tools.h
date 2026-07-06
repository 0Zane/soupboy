#pragma once

#include <Arduino.h>

enum class WiFiScanState : uint8_t {
  Idle,
  Scanning,
  Complete,
  Failed,
};

enum class WiFiNameState : uint8_t {
  Idle,
  Running,
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

void wifiNameUpdate();
bool wifiNameToggle();
bool wifiNameStart();
void wifiNameStop();
WiFiNameState wifiNameState();
const char *wifiNameStateText();
const char *wifiNameCurrentSSID();
uint32_t wifiNameSentCount();
uint8_t wifiNameChannel();
uint16_t wifiNameDelayMs();
uint8_t wifiNameTotalCount();
