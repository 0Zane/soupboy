#include "include/screen.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <string.h>

#include "assets/soup_avatar.h"
#include "include/battery.h"
#include "include/gps.h"
#include "include/ir_tools.h"
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
constexpr uint16_t COL_RED = rgb(236, 78, 58);
constexpr uint16_t COL_TEXT = rgb(235, 231, 196);
constexpr uint16_t COL_DARK_TEXT = rgb(6, 12, 8);

constexpr uint8_t kTabCount = 3;
constexpr uint32_t kFrameMs = 145;
constexpr int16_t kTabBarHeight = 22;
constexpr int16_t kFooterHeight = 14;

Adafruit_ST7735 tft(PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RST);

enum class View : uint8_t {
  Navbar,
  Submenu,
};

enum class Tab : uint8_t {
  Tools,
  Device,
  Rf,
};

enum class EntryKind : uint8_t {
  WifiScan,
  Gps,
  IrTool,
  Avatar,
  Status,
  Battery,
  SystemInfo,
  About,
  RfSafe,
};

struct MenuEntry {
  const char *label;
  EntryKind kind;
};

View view = View::Navbar;
uint8_t tabIndex = static_cast<uint8_t>(Tab::Tools);
uint8_t selectedByTab[kTabCount] = {0, 0, 0};
EntryKind activeEntry = EntryKind::Gps;
uint8_t avatarLine = 0;
bool dirty = true;
uint32_t lastFrameAt = 0;
WiFiScanState lastWifiDrawState = WiFiScanState::Idle;
int lastWifiDrawCount = -1;
IRToolState lastIrDrawState = IRToolState::Idle;

const char *const tabLabels[kTabCount] = {
  "TOOLS",
  "DEVICE",
  "RF",
};

const MenuEntry toolItems[] = {
  {"GPS", EntryKind::Gps},
  {"IR Tool", EntryKind::IrTool},
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
  {"WiFi Scan", EntryKind::WifiScan},
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

void centerText(int16_t y, const String &text, uint16_t color, uint8_t size = 1) {
  const int16_t width = text.length() * 6 * size;
  textAt((screenW() - width) / 2, y, text, color, size);
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

void drawAvatarBitmap(int16_t x, int16_t y) {
  tft.drawRGBBitmap(x, y, SOUP_AVATAR_RGB565, SOUP_AVATAR_WIDTH, SOUP_AVATAR_HEIGHT);
}

void transitionWipe() {
  for (int16_t x = 0; x < screenW(); x += 14) {
    tft.fillRect(x, 0, 7, screenH(), COL_GREEN_DIM);
    delay(7);
  }
}

uint8_t ledForEntry(EntryKind entry) {
  if (entry == EntryKind::WifiScan || entry == EntryKind::RfSafe) {
    return 0;
  }
  if (entry == EntryKind::Gps || entry == EntryKind::IrTool) {
    return 1;
  }
  return 2;
}

void enterSubmenu() {
  activeEntry = selectedEntry().kind;
  view = View::Submenu;
  dirty = true;
}

void showNavbar() {
  view = View::Navbar;
  dirty = true;
}

void moveSelection(int8_t delta) {
  const Tab tab = activeTab();
  uint8_t &selected = selectedFor(tab);
  const uint8_t count = entryCountFor(tab);
  selected = (selected + count + delta) % count;
  dirty = true;

  if (view == View::Submenu) {
    activeEntry = selectedEntry().kind;
  }
}

void moveTab(int8_t delta) {
  tabIndex = (tabIndex + kTabCount + delta) % kTabCount;
  dirty = true;
}

void activateEntry() {
  ledSignalFeature(ledForEntry(activeEntry));

  if (activeEntry == EntryKind::WifiScan) {
    wifiScanStart();
  } else if (activeEntry == EntryKind::IrTool) {
    irToolActivate();
  } else if (activeEntry == EntryKind::Avatar) {
    avatarLine = (avatarLine + 1) % (sizeof(avatarLines) / sizeof(avatarLines[0]));
  } else if (activeEntry == EntryKind::Gps) {
    updateGPS();
  } else if (activeEntry == EntryKind::RfSafe) {
    nrfBegin();
  }

  dirty = true;
}

void handleNavbar(InputEvent event) {
  if (event == InputEvent::Left) {
    moveTab(-1);
  } else if (event == InputEvent::Right) {
    moveTab(1);
  } else if (event == InputEvent::Select) {
    enterSubmenu();
  } else if (event == InputEvent::Home) {
    showNavbar();
  }
}

void handleSubmenu(InputEvent event) {
  if (event == InputEvent::Home) {
    showNavbar();
    return;
  }

  if (event == InputEvent::Left) {
    moveSelection(-1);
  } else if (event == InputEvent::Right) {
    moveSelection(1);
  } else if (event == InputEvent::Select) {
    activateEntry();
  }
}

void handleEvent(InputEvent event) {
  if (event == InputEvent::None) {
    return;
  }

  if (view == View::Navbar) {
    handleNavbar(event);
  } else {
    handleSubmenu(event);
  }
}

void drawNavbar() {
  clearScreen();
  drawTabBar();

  const Tab tab = activeTab();
  centerText(45, tabLabels[tabIndex], COL_AMBER, 2);
  centerText(72, "SELECT TO ENTER", COL_TEXT);
  centerText(90, String(entryCountFor(tab)) + " FEATURES", COL_GREEN_DIM);

  drawFooter("L/R TAB  SEL ENTER");
}

void drawSubmenuSelector() {
  const Tab tab = activeTab();
  const uint8_t selected = selectedFor(tab);
  const uint8_t count = entryCountFor(tab);
  const MenuEntry &entry = selectedEntry();

  tft.fillRect(0, kTabBarHeight, screenW(), 28, COL_PANEL_2);
  tft.drawFastHLine(0, kTabBarHeight + 28, screenW(), COL_GREEN_DIM);
  textAt(9, kTabBarHeight + 9, "<", COL_GREEN);
  textAt(screenW() - 15, kTabBarHeight + 9, ">", COL_GREEN);

  tft.fillRoundRect(25, kTabBarHeight + 5, screenW() - 50, 18, 3, COL_AMBER);
  centerText(kTabBarHeight + 10, clipped(String(entry.label), 18), COL_DARK_TEXT);

  const String countText = String(selected + 1) + "/" + String(count);
  textAt(screenW() - 26, kTabBarHeight + 31, countText, COL_GREEN_DIM);
}

void drawSubmenuContent() {
  const int16_t y0 = 60;
  const EntryKind entry = activeEntry;

  if (entry == EntryKind::WifiScan) {
    textAt(8, y0, "Passive scan only", COL_AMBER);
    const WiFiScanState state = wifiScanState();
    if (state == WiFiScanState::Idle) {
      textAt(8, y0 + 14, "Press select to scan", COL_TEXT);
    } else if (state == WiFiScanState::Scanning) {
      textAt(8, y0 + 14, "Scanning...", COL_GREEN);
    } else if (state == WiFiScanState::Failed) {
      textAt(8, y0 + 14, "Scan failed", COL_RED);
    } else {
      textAt(8, y0 + 14, String("Networks: ") + String(wifiScanCount()), COL_GREEN);
      const int count = wifiScanCount() < 2 ? wifiScanCount() : 2;
      for (int i = 0; i < count; ++i) {
        const int16_t y = y0 + 30 + i * 12;
        String ssid = clipped(wifiSSID(i), 13);
        if (ssid.length() == 0) {
          ssid = "<hidden>";
        }
        textAt(8, y, ssid, COL_TEXT);
        textAt(102, y, String(wifiRSSI(i)) + "dB", COL_GREEN_DIM);
      }
    }
  } else if (entry == EntryKind::Gps) {
    if (gpsCharsProcessed() == 0) {
      textAt(8, y0, "GPS module offline", COL_AMBER);
      textAt(8, y0 + 15, "Waiting for NMEA", COL_TEXT);
    } else if (!isGPSValid()) {
      textAt(8, y0, "Fix awaiting lock", COL_AMBER);
      textAt(8, y0 + 15, String("Satellites: ") + String(getSatellites()), COL_TEXT);
      textAt(8, y0 + 30, String("HDOP: ") + String(getHDOP(), 1), COL_TEXT);
    } else {
      textAt(8, y0, "Fix valid", COL_GREEN);
      textAt(8, y0 + 14, String("Lat ") + String(getLatitude(), 4), COL_TEXT);
      textAt(8, y0 + 28, String("Lon ") + String(getLongitude(), 4), COL_TEXT);
      textAt(8, y0 + 42, String("Sat ") + String(getSatellites()), COL_TEXT);
    }
  } else if (entry == EntryKind::IrTool) {
    textAt(8, y0, "IR read/copy tool", COL_GREEN);
    textAt(8, y0 + 15, String("State: ") + irToolStateText(), COL_TEXT);
    textAt(8, y0 + 30, irToolHasCapture() ? "Stored: yes" : "Stored: no", irToolHasCapture() ? COL_AMBER : COL_GREEN_DIM);
    textAt(8, y0 + 45, String("OUT GPIO") + String(PIN_IR_OUT), COL_GREEN_DIM);
    textAt(94, y0 + 45, String("V") + String(PIN_IR_VCC) + " G" + String(PIN_IR_GND), COL_GREEN_DIM);
  } else if (entry == EntryKind::Avatar) {
    drawAvatarBitmap(10, y0 - 2);
    textAt(67, y0 + 3, clipped(String(avatarLines[avatarLine]), 14), COL_TEXT);
    textAt(67, y0 + 19, "Select changes line", COL_GREEN_DIM);
    textAt(67, y0 + 35, "Soup ready", COL_AMBER);
  } else if (entry == EntryKind::Status) {
    const NrfStatus rf = nrfStatus();
    textAt(8, y0, String("Uptime: ") + uptimeString(), COL_TEXT);
    textAt(8, y0 + 14, String("Battery: ") + batteryStatusText(), COL_TEXT);
    textAt(8, y0 + 28, String("GPS: ") + (isGPSValid() ? "fix" : "standby"), COL_TEXT);
    textAt(8, y0 + 42, String("RF: ") + (rf.txEnabled ? "TX on" : "safe"), COL_GREEN);
  } else if (entry == EntryKind::Battery) {
    const float volts = batteryVoltage();
    const int pct = batteryPercent();
    textAt(8, y0, pct >= 0 ? String("Battery: ") + String(pct) + "%" : String("Battery: offline"), COL_TEXT);
    textAt(8, y0 + 16, String("Voltage: ") + String(volts, 2) + "V", COL_TEXT);
    textAt(8, y0 + 32, String("Status: ") + batteryStatusText(), COL_AMBER);
    const int barWidth = pct >= 0 ? map(pct, 0, 100, 0, 70) : 0;
    tft.drawRect(83, y0 + 2, 74, 12, COL_GREEN_DIM);
    tft.fillRect(85, y0 + 4, barWidth, 8, pct >= 0 && pct < 20 ? COL_RED : COL_GREEN);
  } else if (entry == EntryKind::SystemInfo) {
    textAt(8, y0, SOUPBOY_BUILD_NAME, COL_AMBER);
    textAt(8, y0 + 15, "MCU: ESP32-S3", COL_TEXT);
    textAt(8, y0 + 30, String("Display: ") + String(screenW()) + "x" + String(screenH()), COL_TEXT);
    textAt(8, y0 + 45, "RF TX: disabled", COL_GREEN);
  } else if (entry == EntryKind::About) {
    centerText(y0, "SOUPBOY", COL_AMBER, 2);
    centerText(y0 + 25, "Hardware Pirates", COL_GREEN);
    centerText(y0 + 40, "Fallout Hackathon", COL_TEXT);
  } else if (entry == EntryKind::RfSafe) {
    const NrfStatus rf = nrfStatus();
    textAt(8, y0, "RF TX disabled", COL_GREEN);
    textAt(8, y0 + 15, rf.moduleDetected ? "Module detected" : "Module offline", COL_TEXT);
    textAt(8, y0 + 30, clipped(String(rf.message), 19), COL_GREEN_DIM);
    textAt(8, y0 + 45, "Select refreshes", COL_AMBER);
  }
}

void drawSubmenu() {
  clearScreen();
  drawTabBar();
  drawSubmenuSelector();
  drawSubmenuContent();
  drawFooter("L/R ITEM SEL RUN HOLD NAV");
}

bool isAnimated() {
  return false;
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
  if (view == View::Submenu && activeEntry == EntryKind::WifiScan &&
      (wifiScanState() != lastWifiDrawState || wifiScanCount() != lastWifiDrawCount)) {
    dirty = true;
  }
  if (view == View::Submenu && activeEntry == EntryKind::IrTool &&
      irToolState() != lastIrDrawState) {
    dirty = true;
  }
  handleEvent(event);

  const uint32_t now = millis();
  if (!dirty && (!isAnimated() || now - lastFrameAt < kFrameMs)) {
    return;
  }

  lastFrameAt = now;
  dirty = false;

  if (view == View::Navbar) {
    drawNavbar();
  } else {
    drawSubmenu();
  }

  lastWifiDrawState = wifiScanState();
  lastWifiDrawCount = wifiScanCount();
  lastIrDrawState = irToolState();
}

const char *screenBuildName() {
  return SOUPBOY_BUILD_NAME;
}
