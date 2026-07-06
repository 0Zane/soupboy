#include "include/screen.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#include "assets/soup_avatar.h"
#include "include/battery.h"
#include "include/gps.h"
#include "include/led.h"
#include "include/nrf.h"
#include "include/pins.h"
#include "include/wifi_tools.h"

namespace {

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

constexpr uint16_t COL_BG = rgb(4, 9, 6);
constexpr uint16_t COL_PANEL = rgb(9, 22, 14);
constexpr uint16_t COL_PANEL_2 = rgb(14, 31, 19);
constexpr uint16_t COL_GREEN = rgb(84, 255, 128);
constexpr uint16_t COL_GREEN_DIM = rgb(44, 116, 62);
constexpr uint16_t COL_AMBER = rgb(255, 188, 70);
constexpr uint16_t COL_AMBER_DIM = rgb(132, 85, 34);
constexpr uint16_t COL_RED = rgb(236, 78, 58);
constexpr uint16_t COL_TEXT = rgb(235, 231, 196);
constexpr uint16_t COL_DARK_TEXT = rgb(6, 12, 8);

constexpr uint8_t kHomeCount = 4;
constexpr uint8_t kToolCount = 7;
constexpr uint32_t kFrameMs = 145;
constexpr uint32_t kWifiRefreshMs = 15000;

Adafruit_ST7735 tft(PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RST);

enum class Page : uint8_t {
  Home,
  Avatar,
  Tools,
  ToolDetail,
  Status,
  About,
};

enum class ToolPage : uint8_t {
  RfSafe,
  WifiScan,
  BleSafe,
  Gps,
  Light,
  Battery,
  SystemInfo,
};

Page page = Page::Home;
uint8_t homeIndex = 0;
uint8_t toolIndex = 0;
uint8_t avatarLine = 0;
uint8_t lightMode = 0;
bool dirty = true;
uint32_t lastFrameAt = 0;

const char *const homeItems[kHomeCount] = {
  "Avatar",
  "Tools",
  "Status",
  "About",
};

const char *const toolItems[kToolCount] = {
  "RF Tools",
  "WiFi Scan",
  "BLE Safe",
  "GPS",
  "Light/Laser",
  "Battery",
  "System Info",
};

const char *const avatarLines[] = {
  "Soup level: stable",
  "Mood: cozy",
  "Radiation: probably fine",
  "Today: don't get cooked",
};

void clearScreen();
void drawHome();
void drawAvatar();
void drawTools();
void drawToolDetail();
void drawStatus();
void drawAbout();
void drawRfSafe();
void drawWifiScan();
void drawBleSafe();
void drawGpsTool();
void drawLightTool();
void drawBatteryTool();
void drawSystemTool();

String uptimeString() {
  const uint32_t total = millis() / 1000;
  const uint16_t hours = total / 3600;
  const uint8_t minutes = (total / 60) % 60;
  const uint8_t seconds = total % 60;

  char buffer[12];
  snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
  return String(buffer);
}

String clipped(String text, uint8_t maxLen) {
  if (text.length() <= maxLen) {
    return text;
  }
  return text.substring(0, maxLen - 1) + "~";
}

void setText(uint16_t color, uint8_t size = 1) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.setTextWrap(false);
}

void textAt(int16_t x, int16_t y, const char *text, uint16_t color, uint8_t size = 1) {
  setText(color, size);
  tft.setCursor(x, y);
  tft.print(text);
}

void textAt(int16_t x, int16_t y, const String &text, uint16_t color, uint8_t size = 1) {
  setText(color, size);
  tft.setCursor(x, y);
  tft.print(text);
}

void centerText(int16_t y, const char *text, uint16_t color, uint8_t size = 1) {
  const int16_t width = strlen(text) * 6 * size;
  textAt((SCREEN_WIDTH - width) / 2, y, text, color, size);
}

void drawScanlines() {
  for (int16_t y = 4; y < SCREEN_HEIGHT; y += 6) {
    tft.drawFastHLine(1, y, SCREEN_WIDTH - 2, rgb(2, 17, 10));
  }
}

void drawCornerBrackets(uint16_t color) {
  tft.drawFastHLine(0, 0, 14, color);
  tft.drawFastVLine(0, 0, 14, color);
  tft.drawFastHLine(SCREEN_WIDTH - 14, 0, 14, color);
  tft.drawFastVLine(SCREEN_WIDTH - 1, 0, 14, color);
  tft.drawFastHLine(0, SCREEN_HEIGHT - 1, 14, color);
  tft.drawFastVLine(0, SCREEN_HEIGHT - 14, 14, color);
  tft.drawFastHLine(SCREEN_WIDTH - 14, SCREEN_HEIGHT - 1, 14, color);
  tft.drawFastVLine(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 14, 14, color);
}

void clearScreen() {
  tft.fillScreen(COL_BG);
  drawScanlines();
  drawCornerBrackets(COL_GREEN_DIM);
}

void drawHeader(const char *title, const char *status) {
  tft.fillRect(0, 0, SCREEN_WIDTH, 22, COL_PANEL);
  tft.drawFastHLine(0, 22, SCREEN_WIDTH, COL_GREEN_DIM);
  textAt(5, 6, title, COL_GREEN);
  const int16_t statusWidth = strlen(status) * 6;
  textAt(SCREEN_WIDTH - statusWidth - 5, 6, status, COL_AMBER);
}

void drawFooter(const char *text) {
  tft.fillRect(0, 146, SCREEN_WIDTH, 14, COL_PANEL);
  tft.drawFastHLine(0, 145, SCREEN_WIDTH, COL_GREEN_DIM);
  centerText(149, text, COL_GREEN_DIM);
}

void drawMenuRow(int16_t y, const char *label, bool selected) {
  if (selected) {
    tft.fillRoundRect(6, y - 2, 116, 15, 2, COL_AMBER);
    textAt(12, y + 2, ">", COL_DARK_TEXT);
    textAt(24, y + 2, label, COL_DARK_TEXT);
  } else {
    tft.drawRoundRect(6, y - 2, 116, 15, 2, COL_GREEN_DIM);
    textAt(24, y + 2, label, COL_TEXT);
  }
}

void transitionWipe() {
  for (int16_t x = 0; x < SCREEN_WIDTH; x += 12) {
    tft.fillRect(x, 0, 6, SCREEN_HEIGHT, COL_GREEN_DIM);
    delay(8);
  }
}

void goTo(Page nextPage) {
  if (page != nextPage) {
    transitionWipe();
  }
  page = nextPage;
  dirty = true;
}

void goToToolDetail() {
  if (static_cast<ToolPage>(toolIndex) == ToolPage::WifiScan &&
      wifiScanState() != WiFiScanState::Scanning &&
      (millis() - wifiLastScanMs() > kWifiRefreshMs || wifiLastScanMs() == 0)) {
    wifiScanStart();
  }
  goTo(Page::ToolDetail);
}

void applyLightMode() {
  digitalWrite(PIN_SIGNAL_OUT, LOW);
  ledSet(1, lightMode == 1);
  ledSet(2, lightMode == 2);
}

void handleHome(InputEvent event) {
  if (event == InputEvent::Previous) {
    homeIndex = (homeIndex + kHomeCount - 1) % kHomeCount;
    dirty = true;
  } else if (event == InputEvent::Next) {
    homeIndex = (homeIndex + 1) % kHomeCount;
    dirty = true;
  } else if (event == InputEvent::Select) {
    switch (homeIndex) {
      case 0: goTo(Page::Avatar); break;
      case 1: goTo(Page::Tools); break;
      case 2: goTo(Page::Status); break;
      case 3: goTo(Page::About); break;
    }
  }
}

void handleTools(InputEvent event) {
  if (event == InputEvent::Previous) {
    toolIndex = (toolIndex + kToolCount - 1) % kToolCount;
    dirty = true;
  } else if (event == InputEvent::Next) {
    toolIndex = (toolIndex + 1) % kToolCount;
    dirty = true;
  } else if (event == InputEvent::Select) {
    goToToolDetail();
  } else if (event == InputEvent::Back) {
    goTo(Page::Home);
  }
}

void handleToolDetail(InputEvent event) {
  if (event == InputEvent::Back) {
    goTo(Page::Tools);
    return;
  }

  const ToolPage tool = static_cast<ToolPage>(toolIndex);
  if (event == InputEvent::Select && tool == ToolPage::WifiScan) {
    wifiScanStart();
    dirty = true;
  } else if (event == InputEvent::Select && tool == ToolPage::Light) {
    lightMode = (lightMode + 1) % 3;
    applyLightMode();
    dirty = true;
  } else if (event == InputEvent::Select) {
    goTo(Page::Tools);
  }
}

void handleEvent(InputEvent event) {
  if (event == InputEvent::None) {
    return;
  }

  if (page == Page::Home) {
    handleHome(event);
  } else if (page == Page::Tools) {
    handleTools(event);
  } else if (page == Page::ToolDetail) {
    handleToolDetail(event);
  } else if (page == Page::Avatar) {
    if (event == InputEvent::Previous || event == InputEvent::Next) {
      avatarLine = (avatarLine + 1) % (sizeof(avatarLines) / sizeof(avatarLines[0]));
      dirty = true;
    } else if (event == InputEvent::Select || event == InputEvent::Back) {
      goTo(Page::Home);
    }
  } else if (event == InputEvent::Select || event == InputEvent::Back) {
    goTo(Page::Home);
  }
}

void drawAvatarBitmap(int16_t x, int16_t y) {
  tft.drawRGBBitmap(x, y, SOUP_AVATAR_RGB565, SOUP_AVATAR_WIDTH, SOUP_AVATAR_HEIGHT);
}

void drawHome() {
  clearScreen();
  drawHeader("SOUPBOY", "READY");

  const int16_t bob = (millis() / 420) % 2;
  drawAvatarBitmap(76, 29 + bob);
  textAt(8, 29, "SURVIVOR", COL_AMBER);
  textAt(8, 40, "MODE", COL_AMBER);
  tft.drawFastHLine(8, 54, 55, COL_GREEN_DIM);
  textAt(8, 59, "Fallout kit", COL_GREEN_DIM);

  for (uint8_t i = 0; i < kHomeCount; ++i) {
    drawMenuRow(80 + i * 16, homeItems[i], i == homeIndex);
  }

  drawFooter("STAY WARM. STAY SOUPY.");
}

void drawSteam(int16_t x, int16_t y) {
  const uint8_t phase = (millis() / 220) % 4;
  for (uint8_t i = 0; i < 3; ++i) {
    const int16_t sx = x + 14 + i * 10;
    const int16_t sy = y - 3 - ((phase + i) % 4);
    tft.drawPixel(sx, sy, COL_AMBER_DIM);
    tft.drawFastVLine(sx + 1, sy - 3, 3, COL_GREEN_DIM);
  }
}

void drawAvatar() {
  clearScreen();
  drawHeader("SOUP", "COZY");

  const int16_t x = (SCREEN_WIDTH - SOUP_AVATAR_WIDTH) / 2;
  const int16_t y = 31 + ((millis() / 360) % 3);
  drawSteam(x, y);
  drawAvatarBitmap(x, y);

  if ((millis() / 180) % 22 == 0) {
    tft.drawFastHLine(x + 16, y + 21, 6, rgb(130, 88, 84));
    tft.drawFastHLine(x + 34, y + 21, 6, rgb(130, 88, 84));
  }

  tft.fillRoundRect(8, 93, 112, 37, 3, COL_PANEL_2);
  tft.drawRoundRect(8, 93, 112, 37, 3, COL_GREEN_DIM);
  textAt(13, 101, avatarLines[avatarLine], COL_TEXT);
  textAt(13, 115, "tiny soup guardian", COL_GREEN_DIM);

  drawFooter("RADIATION: PROBABLY FINE");
}

void drawTools() {
  clearScreen();
  drawHeader("TOOLS", "SAFE");

  for (uint8_t i = 0; i < kToolCount; ++i) {
    drawMenuRow(29 + i * 16, toolItems[i], i == toolIndex);
  }

  drawFooter("TX LOCKED BY DEFAULT");
}

void drawRfSafe() {
  clearScreen();
  drawHeader("RF SAFE", "LOCK");

  const NrfStatus rf = nrfStatus();
  textAt(8, 31, "RF SAFE MODE", COL_AMBER);
  textAt(8, 46, "TX: disabled", COL_GREEN);
  textAt(8, 59, "Monitor: standby", COL_TEXT);
  textAt(8, 72, "No transmit tools", COL_TEXT);
  textAt(8, 85, rf.moduleDetected ? "Module: detected" : "Module: offline", COL_TEXT);
  textAt(8, 98, rf.message, COL_GREEN_DIM);

  const uint8_t phase = (millis() / 140) % 6;
  for (uint8_t i = 0; i < 6; ++i) {
    const int16_t h = 5 + ((i + phase) % 6) * 4;
    tft.drawRect(82 + i * 6, 122 - h, 4, h, COL_GREEN_DIM);
    tft.fillRect(83 + i * 6, 121 - h, 2, h - 1, i < 3 ? COL_GREEN : COL_AMBER);
  }

  drawFooter(nrfSafeModeText());
}

void drawWifiScan() {
  clearScreen();
  drawHeader("WIFI SCAN", "PASSIVE");

  textAt(8, 30, "Scan only. No TX tools.", COL_AMBER);

  const WiFiScanState state = wifiScanState();
  if (state == WiFiScanState::Idle) {
    textAt(8, 47, "State: idle", COL_TEXT);
  } else if (state == WiFiScanState::Scanning) {
    textAt(8, 47, "State: scanning", COL_GREEN);
  } else if (state == WiFiScanState::Failed) {
    textAt(8, 47, "State: failed", COL_RED);
  } else {
    textAt(8, 47, "Networks:", COL_GREEN);
  }

  if (state == WiFiScanState::Complete) {
    const int count = wifiScanCount() < 5 ? wifiScanCount() : 5;
    for (int i = 0; i < count; ++i) {
      const int16_t y = 61 + i * 15;
      String ssid = clipped(wifiSSID(i), 12);
      if (ssid.length() == 0) {
        ssid = "<hidden>";
      }
      textAt(8, y, ssid, COL_TEXT);
      textAt(82, y, String(wifiRSSI(i)) + "dB", COL_GREEN_DIM);
    }
    if (wifiScanCount() == 0) {
      textAt(8, 65, "No networks found", COL_TEXT);
    }
  } else if (state == WiFiScanState::Scanning) {
    const uint8_t phase = (millis() / 120) % 9;
    for (uint8_t i = 0; i < 9; ++i) {
      tft.drawFastVLine(16 + i * 10, 111 - ((i + phase) % 5) * 4, 12, COL_GREEN_DIM);
    }
  }

  drawFooter("SELECT RESCANS");
}

void drawBleSafe() {
  clearScreen();
  drawHeader("BLE SAFE", "LOCK");

  textAt(8, 33, "BLE TX: disabled", COL_AMBER);
  textAt(8, 50, "Passive BLE: offline", COL_TEXT);
  textAt(8, 67, "Diagnostics only", COL_TEXT);
  textAt(8, 84, "Demo placeholder only", COL_GREEN_DIM);

  tft.drawRoundRect(22, 108, 84, 22, 4, COL_GREEN_DIM);
  textAt(36, 116, "SAFE MODE", COL_GREEN);

  drawFooter("NO TRANSMIT ACTIONS");
}

void drawGpsTool() {
  clearScreen();
  drawHeader("GPS", "NAV");

  if (gpsCharsProcessed() == 0) {
    textAt(8, 33, "Module: offline", COL_AMBER);
    textAt(8, 50, "Waiting for NMEA", COL_TEXT);
  } else if (!isGPSValid()) {
    textAt(8, 33, "Fix: awaiting lock", COL_AMBER);
    textAt(8, 50, String("Satellites: ") + String(getSatellites()), COL_TEXT);
    textAt(8, 67, String("HDOP: ") + String(getHDOP(), 1), COL_TEXT);
  } else {
    textAt(8, 33, "Fix: valid", COL_GREEN);
    textAt(8, 50, String("Lat ") + String(getLatitude(), 4), COL_TEXT);
    textAt(8, 65, String("Lon ") + String(getLongitude(), 4), COL_TEXT);
    textAt(8, 80, String("Sat ") + String(getSatellites()), COL_TEXT);
    textAt(8, 95, String("Spd ") + String(getSpeed(), 1) + "km/h", COL_TEXT);
  }

  drawFooter("GPS FAILS GRACEFULLY");
}

void drawLightTool() {
  clearScreen();
  drawHeader("LIGHT", "LOCK");

  const char *modeText = "standby";
  if (lightMode == 1) {
    modeText = "beacon A";
  } else if (lightMode == 2) {
    modeText = "beacon B";
  }

  textAt(8, 33, "LED beacon:", COL_GREEN);
  textAt(78, 33, modeText, COL_TEXT);
  textAt(8, 52, "Laser: locked", COL_AMBER);
  textAt(8, 69, "SIG GPIO14: LOW", COL_TEXT);
  textAt(8, 88, "Awaiting calibration", COL_GREEN_DIM);

  tft.drawCircle(66, 120, 14, lightMode == 0 ? COL_GREEN_DIM : COL_AMBER);
  if (lightMode != 0) {
    tft.fillCircle(66, 120, 8, COL_AMBER_DIM);
  }

  drawFooter("SELECT CYCLES LED");
}

void drawBatteryTool() {
  clearScreen();
  drawHeader("BATTERY", "PWR");

  const float volts = batteryVoltage();
  const int pct = batteryPercent();

  textAt(8, 34, "Sense pin: GPIO2", COL_GREEN_DIM);
  textAt(8, 53, String("Voltage: ") + String(volts, 2) + "V", COL_TEXT);
  textAt(8, 70, pct >= 0 ? String("Charge: ") + String(pct) + "% est" : String("Charge: offline"), COL_TEXT);
  textAt(8, 87, String("Status: ") + String(batteryStatusText()), COL_AMBER);

  const int barWidth = pct >= 0 ? map(pct, 0, 100, 0, 92) : 0;
  tft.drawRect(16, 112, 96, 12, COL_GREEN_DIM);
  tft.fillRect(18, 114, barWidth, 8, pct < 20 ? COL_RED : COL_GREEN);

  drawFooter("DIVIDER NEEDS CAL");
}

void drawSystemTool() {
  clearScreen();
  drawHeader("SYSTEM", "INFO");

  textAt(8, 31, SOUPBOY_BUILD_NAME, COL_AMBER);
  textAt(8, 48, "MCU: ESP32-S3", COL_TEXT);
  textAt(8, 63, "Display: 128x160", COL_TEXT);
  textAt(8, 78, "Driver: ST7735", COL_TEXT);
  textAt(8, 93, String("Uptime: ") + uptimeString(), COL_TEXT);
  textAt(8, 108, "RF TX: disabled", COL_GREEN);

  drawFooter("FIRMWARE PROTOTYPE");
}

void drawToolDetail() {
  switch (static_cast<ToolPage>(toolIndex)) {
    case ToolPage::RfSafe: drawRfSafe(); break;
    case ToolPage::WifiScan: drawWifiScan(); break;
    case ToolPage::BleSafe: drawBleSafe(); break;
    case ToolPage::Gps: drawGpsTool(); break;
    case ToolPage::Light: drawLightTool(); break;
    case ToolPage::Battery: drawBatteryTool(); break;
    case ToolPage::SystemInfo: drawSystemTool(); break;
  }
}

void drawStatus() {
  clearScreen();
  drawHeader("STATUS", "ONLINE");

  const NrfStatus rf = nrfStatus();
  textAt(8, 32, SOUPBOY_BUILD_NAME, COL_AMBER);
  textAt(8, 49, String("Uptime: ") + uptimeString(), COL_TEXT);
  textAt(8, 64, String("Battery: ") + String(batteryStatusText()), COL_TEXT);
  textAt(8, 79, String("GPS: ") + String(isGPSValid() ? "fix" : "standby"), COL_TEXT);
  textAt(8, 94, String("RF: ") + String(rf.txEnabled ? "TX on" : "safe"), COL_GREEN);
  textAt(8, 109, "WiFi: passive scan", COL_TEXT);
  textAt(8, 124, "Display: ready", COL_TEXT);

  drawFooter("SURVIVAL SYSTEMS SAFE");
}

void drawAbout() {
  clearScreen();
  drawHeader("ABOUT", "HP");

  centerText(33, "SOUPBOY", COL_AMBER, 2);
  centerText(57, "Hardware Pirates", COL_GREEN);
  centerText(73, "Fallout Hackathon", COL_TEXT);
  centerText(91, "Stay warm.", COL_TEXT);
  centerText(105, "Stay soupy.", COL_TEXT);
  centerText(124, "github/0Zane/soupboy", COL_GREEN_DIM);

  drawFooter("FALLING OUT, SUCCESSFULLY.");
}

}  // namespace

void screenBegin() {
  SPI.begin(PIN_DISPLAY_SCLK, -1, PIN_DISPLAY_MOSI, PIN_DISPLAY_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(SCREEN_ROTATION);
  tft.setTextWrap(false);
  clearScreen();
}

void screenShowBoot() {
  tft.fillScreen(COL_BG);
  drawCornerBrackets(COL_GREEN_DIM);
  centerText(24, "SOUPBOY OS", COL_AMBER, 2);
  centerText(48, "Hardware Pirates", COL_GREEN);
  centerText(63, "Fallout Hackathon", COL_TEXT);

  tft.drawRect(14, 94, 100, 12, COL_GREEN_DIM);
  for (uint8_t step = 0; step <= 100; step += 4) {
    const int width = map(step, 0, 100, 0, 96);
    tft.fillRect(16, 96, width, 8, step < 70 ? COL_GREEN : COL_AMBER);

    if (step % 12 == 0) {
      tft.drawFastHLine(8, 77 + (step % 18), 112, COL_GREEN_DIM);
    }

    textAt(20, 115, "Survival systems", COL_TEXT);
    textAt(38, 128, step < 100 ? "loading..." : "online", step < 100 ? COL_GREEN_DIM : COL_GREEN);
    delay(38);
  }

  delay(350);
  transitionWipe();
  dirty = true;
}

void screenUpdate(InputEvent event) {
  wifiScanUpdate();
  handleEvent(event);

  const uint32_t now = millis();
  const bool animated = page == Page::Home || page == Page::Avatar ||
                        page == Page::ToolDetail;
  if (!dirty && (!animated || now - lastFrameAt < kFrameMs)) {
    return;
  }

  lastFrameAt = now;
  dirty = false;

  switch (page) {
    case Page::Home: drawHome(); break;
    case Page::Avatar: drawAvatar(); break;
    case Page::Tools: drawTools(); break;
    case Page::ToolDetail: drawToolDetail(); break;
    case Page::Status: drawStatus(); break;
    case Page::About: drawAbout(); break;
  }
}

const char *screenBuildName() {
  return SOUPBOY_BUILD_NAME;
}
