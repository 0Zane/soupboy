#include "include/screen.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <string.h>

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

constexpr uint8_t kTabCount = 3;
constexpr uint32_t kFrameMs = 145;
constexpr uint32_t kWifiRefreshMs = 15000;
constexpr int16_t kTabBarHeight = 22;
constexpr int16_t kDetailHeaderHeight = 18;
constexpr int16_t kFooterHeight = 14;

Adafruit_ST7735 tft(PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RST);

enum class View : uint8_t {
  Menu,
  Detail,
};

enum class Tab : uint8_t {
  Tools,
  Device,
  Rf,
};

enum class EntryKind : uint8_t {
  WifiScan,
  Gps,
  Light,
  Avatar,
  Status,
  Battery,
  SystemInfo,
  About,
  RfSafe,
  BleSafe,
};

struct MenuEntry {
  const char *label;
  EntryKind kind;
};

View view = View::Menu;
uint8_t tabIndex = static_cast<uint8_t>(Tab::Tools);
uint8_t selectedByTab[kTabCount] = {0, 0, 0};
EntryKind activeEntry = EntryKind::WifiScan;
uint8_t avatarLine = 0;
uint8_t lightMode = 0;
bool dirty = true;
uint32_t lastFrameAt = 0;

const char *const tabLabels[kTabCount] = {
  "TOOLS",
  "DEVICE",
  "RF",
};

const MenuEntry toolItems[] = {
  {"WiFi Scan", EntryKind::WifiScan},
  {"GPS", EntryKind::Gps},
  {"Light/Laser", EntryKind::Light},
};

const MenuEntry deviceItems[] = {
  {"Avatar", EntryKind::Avatar},
  {"Status", EntryKind::Status},
  {"Battery", EntryKind::Battery},
  {"System Info", EntryKind::SystemInfo},
  {"About", EntryKind::About},
};

const MenuEntry rfItems[] = {
  {"RF Safe", EntryKind::RfSafe},
  {"BLE Safe", EntryKind::BleSafe},
};

const char *const avatarLines[] = {
  "Soup level: stable",
  "Mood: cozy",
  "Radiation: probably fine",
  "Today: don't get cooked",
};

int16_t screenW() {
  return tft.width();
}

int16_t screenH() {
  return tft.height();
}

int16_t footerTop() {
  return screenH() - kFooterHeight;
}

Tab activeTab() {
  return static_cast<Tab>(tabIndex);
}

const MenuEntry *entriesFor(Tab tab) {
  switch (tab) {
    case Tab::Tools: return toolItems;
    case Tab::Device: return deviceItems;
    case Tab::Rf: return rfItems;
  }
  return toolItems;
}

uint8_t entryCountFor(Tab tab) {
  switch (tab) {
    case Tab::Tools: return sizeof(toolItems) / sizeof(toolItems[0]);
    case Tab::Device: return sizeof(deviceItems) / sizeof(deviceItems[0]);
    case Tab::Rf: return sizeof(rfItems) / sizeof(rfItems[0]);
  }
  return 0;
}

uint8_t &selectedFor(Tab tab) {
  return selectedByTab[static_cast<uint8_t>(tab)];
}

const MenuEntry &selectedEntry() {
  const Tab tab = activeTab();
  return entriesFor(tab)[selectedFor(tab)];
}

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
  if (maxLen == 0) {
    return String("");
  }
  if (text.length() <= maxLen) {
    return text;
  }
  if (maxLen == 1) {
    return String("~");
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
  textAt((screenW() - width) / 2, y, text, color, size);
}

void rightText(int16_t y, const char *text, uint16_t color) {
  const int16_t width = strlen(text) * 6;
  textAt(screenW() - width - 6, y, text, color);
}

void drawScanlines() {
  for (int16_t y = 4; y < screenH(); y += 6) {
    tft.drawFastHLine(1, y, screenW() - 2, rgb(2, 17, 10));
  }
}

void drawCornerBrackets(uint16_t color) {
  const int16_t w = screenW();
  const int16_t h = screenH();

  tft.drawFastHLine(0, 0, 14, color);
  tft.drawFastVLine(0, 0, 14, color);
  tft.drawFastHLine(w - 14, 0, 14, color);
  tft.drawFastVLine(w - 1, 0, 14, color);
  tft.drawFastHLine(0, h - 1, 14, color);
  tft.drawFastVLine(0, h - 14, 14, color);
  tft.drawFastHLine(w - 14, h - 1, 14, color);
  tft.drawFastVLine(w - 1, h - 14, 14, color);
}

void clearScreen() {
  tft.fillScreen(COL_BG);
  drawScanlines();
  drawCornerBrackets(COL_GREEN_DIM);
}

void drawFooter(const char *text) {
  tft.fillRect(0, footerTop(), screenW(), kFooterHeight, COL_PANEL);
  tft.drawFastHLine(0, footerTop() - 1, screenW(), COL_GREEN_DIM);
  centerText(footerTop() + 4, text, COL_GREEN_DIM);
}

void drawTabBar() {
  const int16_t baseWidth = screenW() / kTabCount;

  for (uint8_t i = 0; i < kTabCount; ++i) {
    const int16_t x = i * baseWidth;
    const int16_t w = (i == kTabCount - 1) ? screenW() - x : baseWidth;
    const bool selected = i == tabIndex;

    tft.fillRect(x, 0, w, kTabBarHeight, selected ? COL_AMBER : COL_PANEL);
    tft.drawRect(x, 0, w, kTabBarHeight, selected ? COL_AMBER : COL_GREEN_DIM);
    const int16_t labelWidth = strlen(tabLabels[i]) * 6;
    textAt(x + (w - labelWidth) / 2, 7, tabLabels[i], selected ? COL_DARK_TEXT : COL_GREEN);
  }
}

void drawDetailHeader(const char *title, const char *status) {
  drawTabBar();
  tft.fillRect(0, kTabBarHeight, screenW(), kDetailHeaderHeight, COL_PANEL_2);
  tft.drawFastHLine(0, kTabBarHeight + kDetailHeaderHeight, screenW(), COL_GREEN_DIM);
  textAt(7, kTabBarHeight + 6, title, COL_GREEN);
  rightText(kTabBarHeight + 6, status, COL_AMBER);
}

void drawMenuRow(int16_t y, const char *label, bool selected) {
  const int16_t rowX = 8;
  const int16_t rowW = screenW() - 16;

  if (selected) {
    tft.fillRoundRect(rowX, y - 2, rowW, 15, 2, COL_AMBER);
    textAt(rowX + 7, y + 2, ">", COL_DARK_TEXT);
    textAt(rowX + 20, y + 2, clipped(String(label), (rowW - 26) / 6), COL_DARK_TEXT);
  } else {
    tft.drawRoundRect(rowX, y - 2, rowW, 15, 2, COL_GREEN_DIM);
    textAt(rowX + 20, y + 2, clipped(String(label), (rowW - 26) / 6), COL_TEXT);
  }
}

void drawAvatarBitmap(int16_t x, int16_t y) {
  tft.drawRGBBitmap(x, y, SOUP_AVATAR_RGB565, SOUP_AVATAR_WIDTH, SOUP_AVATAR_HEIGHT);
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

void transitionWipe() {
  for (int16_t x = 0; x < screenW(); x += 14) {
    tft.fillRect(x, 0, 7, screenH(), COL_GREEN_DIM);
    delay(7);
  }
}

void applyLightMode() {
  digitalWrite(PIN_SIGNAL_OUT, LOW);
  ledSet(1, lightMode == 1);
  ledSet(2, lightMode == 2);
}

void prepareEntry(EntryKind entry) {
  if (entry == EntryKind::WifiScan &&
      wifiScanState() != WiFiScanState::Scanning &&
      (millis() - wifiLastScanMs() > kWifiRefreshMs || wifiLastScanMs() == 0)) {
    wifiScanStart();
  }
}

void openSelectedEntry() {
  activeEntry = selectedEntry().kind;
  prepareEntry(activeEntry);
  transitionWipe();
  view = View::Detail;
  dirty = true;
}

void showMenu() {
  if (view != View::Menu) {
    transitionWipe();
  }
  view = View::Menu;
  dirty = true;
}

void moveSelection(int8_t delta) {
  const Tab tab = activeTab();
  uint8_t &selected = selectedFor(tab);
  const uint8_t count = entryCountFor(tab);
  selected = (selected + count + delta) % count;
  dirty = true;

  if (view == View::Detail) {
    activeEntry = selectedEntry().kind;
    prepareEntry(activeEntry);
  }
}

void moveTab(int8_t delta) {
  tabIndex = (tabIndex + kTabCount + delta) % kTabCount;
  if (view == View::Detail) {
    activeEntry = selectedEntry().kind;
    prepareEntry(activeEntry);
  }
  dirty = true;
}

void handleMenu(InputEvent event) {
  if (event == InputEvent::Previous) {
    moveSelection(-1);
  } else if (event == InputEvent::Next) {
    moveSelection(1);
  } else if (event == InputEvent::Left) {
    moveTab(-1);
  } else if (event == InputEvent::Right) {
    moveTab(1);
  } else if (event == InputEvent::Select) {
    openSelectedEntry();
  }
}

void handleDetail(InputEvent event) {
  if (event == InputEvent::Back) {
    showMenu();
    return;
  }

  if (event == InputEvent::Previous) {
    if (activeEntry == EntryKind::Avatar) {
      avatarLine = (avatarLine + (sizeof(avatarLines) / sizeof(avatarLines[0])) - 1) %
                   (sizeof(avatarLines) / sizeof(avatarLines[0]));
      dirty = true;
    } else {
      moveSelection(-1);
    }
  } else if (event == InputEvent::Next) {
    if (activeEntry == EntryKind::Avatar) {
      avatarLine = (avatarLine + 1) % (sizeof(avatarLines) / sizeof(avatarLines[0]));
      dirty = true;
    } else {
      moveSelection(1);
    }
  } else if (event == InputEvent::Left) {
    moveTab(-1);
  } else if (event == InputEvent::Right) {
    moveTab(1);
  } else if (event == InputEvent::Select && activeEntry == EntryKind::WifiScan) {
    wifiScanStart();
    dirty = true;
  } else if (event == InputEvent::Select && activeEntry == EntryKind::Light) {
    lightMode = (lightMode + 1) % 3;
    applyLightMode();
    dirty = true;
  } else if (event == InputEvent::Select && activeEntry == EntryKind::Avatar) {
    avatarLine = (avatarLine + 1) % (sizeof(avatarLines) / sizeof(avatarLines[0]));
    dirty = true;
  }
}

void handleEvent(InputEvent event) {
  if (event == InputEvent::None) {
    return;
  }

  if (view == View::Menu) {
    handleMenu(event);
  } else {
    handleDetail(event);
  }
}

void drawMenu() {
  clearScreen();
  drawTabBar();

  const Tab tab = activeTab();
  const MenuEntry *items = entriesFor(tab);
  const uint8_t count = entryCountFor(tab);
  const uint8_t selected = selectedFor(tab);

  for (uint8_t i = 0; i < count; ++i) {
    drawMenuRow(30 + i * 17, items[i].label, i == selected);
  }

  drawFooter("UP/DN ITEM  HOLD L/R TAB");
}

void drawAvatar() {
  clearScreen();
  drawDetailHeader("AVATAR", "COZY");

  const int16_t avatarX = 11;
  const int16_t avatarY = kTabBarHeight + kDetailHeaderHeight + 8 + ((millis() / 360) % 3);
  drawSteam(avatarX, avatarY);
  drawAvatarBitmap(avatarX, avatarY);

  if ((millis() / 180) % 22 == 0) {
    tft.drawFastHLine(avatarX + 16, avatarY + 21, 6, rgb(130, 88, 84));
    tft.drawFastHLine(avatarX + 34, avatarY + 21, 6, rgb(130, 88, 84));
  }

  tft.fillRoundRect(66, 49, 86, 48, 3, COL_PANEL_2);
  tft.drawRoundRect(66, 49, 86, 48, 3, COL_GREEN_DIM);
  textAt(71, 58, clipped(String(avatarLines[avatarLine]), 13), COL_TEXT);
  textAt(71, 72, "tiny soup", COL_GREEN_DIM);
  textAt(71, 84, "guardian", COL_GREEN_DIM);

  drawFooter("NEXT LINE  HOLD SEL BACK");
}

void drawRfSafe() {
  clearScreen();
  drawDetailHeader("RF SAFE", "LOCK");

  const NrfStatus rf = nrfStatus();
  textAt(8, 46, "TX: disabled", COL_GREEN);
  textAt(8, 59, "Monitor: standby", COL_TEXT);
  textAt(8, 72, rf.moduleDetected ? "Module: detected" : "Module: offline", COL_TEXT);
  textAt(8, 85, clipped(String(rf.message), 16), COL_GREEN_DIM);

  const uint8_t phase = (millis() / 140) % 6;
  for (uint8_t i = 0; i < 6; ++i) {
    const int16_t h = 5 + ((i + phase) % 6) * 4;
    tft.drawRect(119 + i * 6, 105 - h, 4, h, COL_GREEN_DIM);
    tft.fillRect(120 + i * 6, 104 - h, 2, h - 1, i < 3 ? COL_GREEN : COL_AMBER);
  }

  drawFooter(nrfSafeModeText());
}

void drawWifiScan() {
  clearScreen();
  drawDetailHeader("WIFI SCAN", "PASSIVE");

  textAt(8, 45, "Scan only. No TX.", COL_AMBER);

  const WiFiScanState state = wifiScanState();
  if (state == WiFiScanState::Idle) {
    textAt(8, 58, "State: idle", COL_TEXT);
  } else if (state == WiFiScanState::Scanning) {
    textAt(8, 58, "State: scanning", COL_GREEN);
  } else if (state == WiFiScanState::Failed) {
    textAt(8, 58, "State: failed", COL_RED);
  } else {
    textAt(8, 58, "Networks:", COL_GREEN);
  }

  if (state == WiFiScanState::Complete) {
    const int count = wifiScanCount() < 4 ? wifiScanCount() : 4;
    for (int i = 0; i < count; ++i) {
      const int16_t y = 72 + i * 11;
      String ssid = clipped(wifiSSID(i), 13);
      if (ssid.length() == 0) {
        ssid = "<hidden>";
      }
      textAt(8, y, ssid, COL_TEXT);
      textAt(100, y, String(wifiRSSI(i)) + "dB", COL_GREEN_DIM);
    }
    if (wifiScanCount() == 0) {
      textAt(8, 74, "No networks found", COL_TEXT);
    }
  } else if (state == WiFiScanState::Scanning) {
    const uint8_t phase = (millis() / 120) % 9;
    for (uint8_t i = 0; i < 9; ++i) {
      tft.drawFastVLine(32 + i * 10, 103 - ((i + phase) % 5) * 4, 12, COL_GREEN_DIM);
    }
  }

  drawFooter("SEL RESCAN  HOLD SEL BACK");
}

void drawBleSafe() {
  clearScreen();
  drawDetailHeader("BLE SAFE", "LOCK");

  textAt(8, 48, "BLE TX: disabled", COL_AMBER);
  textAt(8, 63, "Passive BLE: offline", COL_TEXT);
  textAt(8, 78, "Diagnostics only", COL_TEXT);
  textAt(8, 93, "Demo placeholder", COL_GREEN_DIM);

  drawFooter("HOLD SEL BACK");
}

void drawGpsTool() {
  clearScreen();
  drawDetailHeader("GPS", "NAV");

  if (gpsCharsProcessed() == 0) {
    textAt(8, 49, "Module: offline", COL_AMBER);
    textAt(8, 64, "Waiting for NMEA", COL_TEXT);
  } else if (!isGPSValid()) {
    textAt(8, 49, "Fix: awaiting lock", COL_AMBER);
    textAt(8, 64, String("Satellites: ") + String(getSatellites()), COL_TEXT);
    textAt(8, 79, String("HDOP: ") + String(getHDOP(), 1), COL_TEXT);
  } else {
    textAt(8, 48, "Fix: valid", COL_GREEN);
    textAt(8, 61, String("Lat ") + String(getLatitude(), 4), COL_TEXT);
    textAt(8, 74, String("Lon ") + String(getLongitude(), 4), COL_TEXT);
    textAt(8, 87, String("Sat ") + String(getSatellites()), COL_TEXT);
    textAt(8, 100, String("Spd ") + String(getSpeed(), 1) + "km/h", COL_TEXT);
  }

  drawFooter("HOLD SEL BACK");
}

void drawLightTool() {
  clearScreen();
  drawDetailHeader("LIGHT", "LOCK");

  const char *modeText = "standby";
  if (lightMode == 1) {
    modeText = "beacon A";
  } else if (lightMode == 2) {
    modeText = "beacon B";
  }

  textAt(8, 49, "LED beacon:", COL_GREEN);
  textAt(78, 49, modeText, COL_TEXT);
  textAt(8, 66, "Laser: locked", COL_AMBER);
  textAt(8, 82, "SIG GPIO14: LOW", COL_TEXT);

  tft.drawCircle(132, 88, 14, lightMode == 0 ? COL_GREEN_DIM : COL_AMBER);
  if (lightMode != 0) {
    tft.fillCircle(132, 88, 8, COL_AMBER_DIM);
  }

  drawFooter("SEL CYCLES  HOLD SEL BACK");
}

void drawBatteryTool() {
  clearScreen();
  drawDetailHeader("BATTERY", "PWR");

  const float volts = batteryVoltage();
  const int pct = batteryPercent();

  textAt(8, 47, "Sense pin: GPIO2", COL_GREEN_DIM);
  textAt(8, 61, String("Voltage: ") + String(volts, 2) + "V", COL_TEXT);
  textAt(8, 75, pct >= 0 ? String("Charge: ") + String(pct) + "% est" : String("Charge: offline"), COL_TEXT);
  textAt(8, 89, String("Status: ") + String(batteryStatusText()), COL_AMBER);

  const int barWidth = pct >= 0 ? map(pct, 0, 100, 0, 60) : 0;
  tft.drawRect(91, 101, 64, 12, COL_GREEN_DIM);
  tft.fillRect(93, 103, barWidth, 8, pct < 20 ? COL_RED : COL_GREEN);

  drawFooter("HOLD SEL BACK");
}

void drawSystemTool() {
  clearScreen();
  drawDetailHeader("SYSTEM", "INFO");

  textAt(8, 46, SOUPBOY_BUILD_NAME, COL_AMBER);
  textAt(8, 60, "MCU: ESP32-S3", COL_TEXT);
  textAt(8, 73, String("Display: ") + String(screenW()) + "x" + String(screenH()), COL_TEXT);
  textAt(8, 86, "Driver: ST7735", COL_TEXT);
  textAt(8, 99, String("Uptime: ") + uptimeString(), COL_TEXT);
  textAt(108, 86, "RF TX", COL_GREEN_DIM);
  textAt(108, 99, "OFF", COL_GREEN);

  drawFooter("HOLD SEL BACK");
}

void drawStatus() {
  clearScreen();
  drawDetailHeader("STATUS", "ONLINE");

  const NrfStatus rf = nrfStatus();
  textAt(8, 46, String("Uptime: ") + uptimeString(), COL_TEXT);
  textAt(8, 59, String("Battery: ") + String(batteryStatusText()), COL_TEXT);
  textAt(8, 72, String("GPS: ") + String(isGPSValid() ? "fix" : "standby"), COL_TEXT);
  textAt(8, 85, String("RF: ") + String(rf.txEnabled ? "TX on" : "safe"), COL_GREEN);
  textAt(8, 98, "WiFi: passive scan", COL_TEXT);

  drawFooter("HOLD SEL BACK");
}

void drawAbout() {
  clearScreen();
  drawDetailHeader("ABOUT", "HP");

  centerText(49, "SOUPBOY", COL_AMBER, 2);
  centerText(74, "Hardware Pirates", COL_GREEN);
  centerText(88, "Fallout Hackathon", COL_TEXT);
  centerText(102, "Stay warm. Stay soupy.", COL_TEXT);

  drawFooter("HOLD SEL BACK");
}

void drawDetail() {
  switch (activeEntry) {
    case EntryKind::WifiScan: drawWifiScan(); break;
    case EntryKind::Gps: drawGpsTool(); break;
    case EntryKind::Light: drawLightTool(); break;
    case EntryKind::Avatar: drawAvatar(); break;
    case EntryKind::Status: drawStatus(); break;
    case EntryKind::Battery: drawBatteryTool(); break;
    case EntryKind::SystemInfo: drawSystemTool(); break;
    case EntryKind::About: drawAbout(); break;
    case EntryKind::RfSafe: drawRfSafe(); break;
    case EntryKind::BleSafe: drawBleSafe(); break;
  }
}

bool isAnimated() {
  if (view == View::Menu) {
    return false;
  }

  return activeEntry == EntryKind::Avatar ||
         activeEntry == EntryKind::RfSafe ||
         activeEntry == EntryKind::WifiScan ||
         activeEntry == EntryKind::Light;
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
  centerText(20, "SOUPBOY OS", COL_AMBER, 2);
  centerText(46, "Hardware Pirates", COL_GREEN);
  centerText(61, "Fallout Hackathon", COL_TEXT);

  const int16_t barX = (screenW() - 104) / 2;
  tft.drawRect(barX, 82, 104, 12, COL_GREEN_DIM);
  for (uint8_t step = 0; step <= 100; step += 4) {
    const int width = map(step, 0, 100, 0, 100);
    tft.fillRect(barX + 2, 84, width, 8, step < 70 ? COL_GREEN : COL_AMBER);

    if (step % 12 == 0) {
      tft.drawFastHLine(18, 70 + (step % 10), screenW() - 36, COL_GREEN_DIM);
    }

    centerText(101, "Survival systems", COL_TEXT);
    centerText(114, step < 100 ? "loading..." : "online", step < 100 ? COL_GREEN_DIM : COL_GREEN);
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
  if (!dirty && (!isAnimated() || now - lastFrameAt < kFrameMs)) {
    return;
  }

  lastFrameAt = now;
  dirty = false;

  if (view == View::Menu) {
    drawMenu();
  } else {
    drawDetail();
  }
}

const char *screenBuildName() {
  return SOUPBOY_BUILD_NAME;
}
