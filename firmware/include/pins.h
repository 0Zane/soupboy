#pragma once

#include <Arduino.h>

// SoupBoy pin map from the ESP32-S3 schematic.
// Keep all hardware assumptions here so board revisions only touch one file.

// 1.8" 128x160 SPI display. Rotation 1 turns the full UI for the wrist mount
// so the long side of the display can sit across the hand.
constexpr int PIN_DISPLAY_RST = 8;
constexpr int PIN_DISPLAY_CS = 9;
constexpr int PIN_DISPLAY_DC = 10;
constexpr int PIN_DISPLAY_CS_ALT = 7;  // Schematic has a second CS label; reserved.
constexpr int PIN_DISPLAY_MOSI = 11;
constexpr int PIN_DISPLAY_SCLK = 12;
constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 128;
constexpr int SCREEN_ROTATION = 1;

// Physical buttons. The schematic drives these from a 3V3 ladder, so they are
// treated as active-high with internal pulldowns.
constexpr int PIN_BUTTON_LEFT = 6;
constexpr int PIN_BUTTON_MIDDLE = 41;
constexpr int PIN_BUTTON_RIGHT = 5;
constexpr int PIN_BUTTON_PREV = PIN_BUTTON_LEFT;
constexpr int PIN_BUTTON_SELECT = PIN_BUTTON_MIDDLE;
constexpr int PIN_BUTTON_NEXT = PIN_BUTTON_RIGHT;
constexpr bool BUTTON_ACTIVE_HIGH = true;
constexpr uint16_t BUTTON_DEBOUNCE_MS = 70;
constexpr uint16_t BUTTON_EVENT_GUARD_MS = 220;
constexpr uint16_t BUTTON_LONG_PRESS_MS = 850;

// Battery divider: +BATT -> 100K -> GPIO2/INT -> 100K -> GND.
constexpr int PIN_BATTERY_ADC = 2;
constexpr float BATTERY_DIVIDER_RATIO = 2.0f;
// GPS NEO-6M UART.
constexpr int PIN_GPS_RX = 16;
constexpr int PIN_GPS_TX = 17;
constexpr int PIN_GPS_PPS = 13;
constexpr uint32_t GPS_BAUD = 9600;

// IR module placeholder wiring. Firmware only reads OUT for now; update these
// after the module is soldered. Do not power the final module from GPIO unless
// its current draw is confirmed safe.
constexpr int PIN_IR_OUT = 35;
#define PIN_IR_RECV   PIN_IR_OUT
constexpr int PIN_IR_VCC = 36;
constexpr int PIN_IR_GND = 37;

// Indicator LEDs.
constexpr int PIN_LED1 = 15;
constexpr int PIN_LED2 = 18;
constexpr int PIN_LED3 = 21;

// NRF24L01 header labels exist on the schematic, but CE/CSN/MISO are not
// clearly mapped back to ESP32 pins in the print. RF firmware stays TX-disabled.
constexpr int PIN_NRF_SCK = PIN_DISPLAY_SCLK;
constexpr int PIN_NRF_MOSI = PIN_GPS_PPS;
constexpr int PIN_NRF_MISO = -1;
constexpr int PIN_NRF_CSN = -1;
constexpr int PIN_NRF_CE = -1;
constexpr int PIN_NRF_IRQ = -1;

// Compatibility aliases for older sketch code and common GPS examples.
#define RXD2 PIN_GPS_RX
#define TXD2 PIN_GPS_TX
