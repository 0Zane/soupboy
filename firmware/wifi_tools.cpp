#include "include/wifi_tools.h"

#include <WiFi.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <string.h>

namespace {

constexpr uint8_t kMaxSsidLength = 32;
constexpr uint8_t kNamesPerAdvertiseTick = 3;
constexpr uint16_t kNameAdvertiseDelayMs = 120;
constexpr uint8_t kAdvertiseChannels[] = {1, 6, 11};

const char *const funnyNames[] = {
  "404 WiFi Not Found",
  "418 I'm A Teapot",
  "Github Copilot WiFi",
  "AI Generated WiFi",
  "ChatGPT Hotspot",
  "Blockchain WiFi Node",
  "NFT of This WiFi",
  "Web3 Connection",
  "Metaverse Portal",
  "Quantum WiFi",
  "5G COVID Chip",
  "Bill Gates Tracking",
  "Lag Prime",
  "Ping 999ms",
  "Noob Detector",
  "Pro Gamer WiFi",
  "G Fuel Hotspot",
  "Rage Quit Network",
  "Aimbot WiFi",
  "Wallhack Access Point",
  "Sweaty Tryhard WiFi",
  "This WiFi Is Cringe",
  "Based Department",
  "Ratio + WiFi",
  "L + No WiFi",
  "Touch Grass Network",
  "I'm In Your Walls",
  "Bonk WiFi",
  "Doge Coin Miner",
  "Shiba Inu Hotspot",
  "Burger King Free WiFi",
  "Taco Bell Network",
  "Suspicious Snack Bar",
  "Free Pizza Hut",
  "McDonald's Ice Cream Machine Broken",
  "Starbucks WiFi Overpriced",
  "The Ring WiFi",
  "Candyman Hotspot",
  "Bloody Mary Network",
  "Amityville Access Point",
  "Poltergeist Internet",
  "Marriage Counselor WiFi",
  "Girlfriend's Hotspot",
  "Boyfriend's Secret Network",
  "Ex's New WiFi Better",
  "Rebound Connection",
  "Dating App WiFi",
  "WiFi McWifFace",
  "Spaghetti Code Network",
  "Coffee-Fueled WiFi",
  "Procrastination Station",
  "Homework Avoidance System",
  "3AM Impulse Network",
};

WiFiScanState scanState = WiFiScanState::Idle;
int networkCount = 0;
uint32_t lastScanAt = 0;

WiFiNameState nameState = WiFiNameState::Idle;
uint8_t currentNameIndex = 0;
uint8_t currentChannel = 6;
uint32_t lastAdvertiseAt = 0;
uint32_t nameSentCount = 0;
uint16_t sequenceNumber = 0;
bool advertiserPrepared = false;
char currentSsid[kMaxSsidLength + 1] = "";

uint8_t funnyNameCount() {
  return sizeof(funnyNames) / sizeof(funnyNames[0]);
}

void setStaMode() {
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  advertiserPrepared = false;
}

void stopScanState() {
  WiFi.scanDelete();
  networkCount = 0;
  scanState = WiFiScanState::Idle;
}

bool prepareAdvertiser() {
  if (advertiserPrepared) {
    return true;
  }

  WiFi.scanDelete();
  if (!WiFi.mode(WIFI_AP)) {
    nameState = WiFiNameState::Failed;
    return false;
  }

  if (esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    nameState = WiFiNameState::Failed;
    return false;
  }

  advertiserPrepared = true;
  return true;
}

void copyCurrentSsid(const char *ssid) {
  const size_t length = strlen(ssid);
  const size_t clippedLength = length > kMaxSsidLength ? kMaxSsidLength : length;
  memcpy(currentSsid, ssid, clippedLength);
  currentSsid[clippedLength] = '\0';
}

bool sendBeaconFrame(const char *ssid) {
  const uint8_t ssidLength = strlen(ssid);
  uint8_t mac[6] = {
    0xDE,
    0xAD,
    0xBE,
    0xEF,
    static_cast<uint8_t>(random(0, 256)),
    static_cast<uint8_t>(random(0, 256)),
  };

  esp_wifi_set_mac(WIFI_IF_AP, mac);
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);

  uint8_t beacon[128];
  int index = 0;

  beacon[index++] = 0x80;
  beacon[index++] = 0x00;
  beacon[index++] = 0x00;
  beacon[index++] = 0x00;

  for (uint8_t i = 0; i < 6; ++i) {
    beacon[index++] = 0xFF;
  }
  for (uint8_t i = 0; i < 6; ++i) {
    beacon[index++] = mac[i];
  }
  for (uint8_t i = 0; i < 6; ++i) {
    beacon[index++] = mac[i];
  }

  const uint16_t sequenceControl = (sequenceNumber++ & 0x0FFF) << 4;
  beacon[index++] = sequenceControl & 0xFF;
  beacon[index++] = (sequenceControl >> 8) & 0xFF;

  for (uint8_t i = 0; i < 8; ++i) {
    beacon[index++] = 0x00;
  }

  beacon[index++] = 0x64;
  beacon[index++] = 0x00;
  beacon[index++] = 0x01;
  beacon[index++] = 0x04;

  beacon[index++] = 0x00;
  beacon[index++] = ssidLength;
  memcpy(&beacon[index], ssid, ssidLength);
  index += ssidLength;

  beacon[index++] = 0x01;
  beacon[index++] = 0x08;
  beacon[index++] = 0x82;
  beacon[index++] = 0x84;
  beacon[index++] = 0x8B;
  beacon[index++] = 0x96;
  beacon[index++] = 0x0C;
  beacon[index++] = 0x12;
  beacon[index++] = 0x18;
  beacon[index++] = 0x24;

  beacon[index++] = 0x03;
  beacon[index++] = 0x01;
  beacon[index++] = currentChannel;

  return esp_wifi_80211_tx(WIFI_IF_AP, beacon, index, false) == ESP_OK;
}

bool advertiseNextName() {
  if (!prepareAdvertiser()) {
    return false;
  }

  const uint8_t channelIndex = nameSentCount % (sizeof(kAdvertiseChannels) / sizeof(kAdvertiseChannels[0]));
  currentChannel = kAdvertiseChannels[channelIndex];

  copyCurrentSsid(funnyNames[currentNameIndex]);
  currentNameIndex = (currentNameIndex + 1) % funnyNameCount();

  if (!sendBeaconFrame(currentSsid)) {
    nameState = WiFiNameState::Failed;
    advertiserPrepared = false;
    return false;
  }

  ++nameSentCount;
  return true;
}

}  // namespace

void wifiToolsBegin() {
  randomSeed(esp_random());
  setStaMode();
}

bool wifiScanStart() {
  if (scanState == WiFiScanState::Scanning) {
    return false;
  }

  wifiNameStop();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  WiFi.scanDelete();
  networkCount = 0;
  const int result = WiFi.scanNetworks(true, false);
  lastScanAt = millis();

  if (result >= 0) {
    networkCount = result;
    scanState = WiFiScanState::Complete;
  } else if (result == WIFI_SCAN_RUNNING) {
    scanState = WiFiScanState::Scanning;
  } else {
    scanState = WiFiScanState::Failed;
  }

  return true;
}

void wifiScanUpdate() {
  if (scanState != WiFiScanState::Scanning) {
    return;
  }

  const int result = WiFi.scanComplete();
  if (result >= 0) {
    networkCount = result;
    scanState = WiFiScanState::Complete;
    lastScanAt = millis();
  } else if (result == WIFI_SCAN_FAILED) {
    networkCount = 0;
    scanState = WiFiScanState::Failed;
    lastScanAt = millis();
  }
}

WiFiScanState wifiScanState() {
  return scanState;
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

void wifiNameUpdate() {
  if (nameState != WiFiNameState::Running) {
    return;
  }

  const uint32_t now = millis();
  if (lastAdvertiseAt != 0 && now - lastAdvertiseAt < kNameAdvertiseDelayMs) {
    return;
  }

  lastAdvertiseAt = now;
  for (uint8_t i = 0; i < kNamesPerAdvertiseTick && nameState == WiFiNameState::Running; ++i) {
    advertiseNextName();
  }
}

bool wifiNameToggle() {
  if (nameState == WiFiNameState::Running) {
    wifiNameStop();
    return false;
  }
  return wifiNameStart();
}

bool wifiNameStart() {
  if (nameState == WiFiNameState::Running) {
    return true;
  }

  stopScanState();
  nameState = WiFiNameState::Running;
  lastAdvertiseAt = 0;
  advertiserPrepared = false;
  currentChannel = 6;

  return prepareAdvertiser();
}

void wifiNameStop() {
  if (nameState == WiFiNameState::Idle) {
    return;
  }

  nameState = WiFiNameState::Idle;
  setStaMode();
}

WiFiNameState wifiNameState() {
  return nameState;
}

const char *wifiNameStateText() {
  switch (nameState) {
    case WiFiNameState::Idle: return "idle";
    case WiFiNameState::Running: return "running";
    case WiFiNameState::Failed: return "failed";
  }
  return "unknown";
}

const char *wifiNameCurrentSSID() {
  return currentSsid[0] == '\0' ? "--" : currentSsid;
}

uint32_t wifiNameSentCount() {
  return nameSentCount;
}

uint8_t wifiNameChannel() {
  return currentChannel;
}

uint16_t wifiNameDelayMs() {
  return kNameAdvertiseDelayMs;
}

uint8_t wifiNameTotalCount() {
  return funnyNameCount();
}
