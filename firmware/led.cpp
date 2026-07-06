#include "include/led.h"

#include "include/pins.h"

namespace {

constexpr int ledPins[] = {PIN_LED1, PIN_LED2, PIN_LED3};
constexpr uint32_t kFeatureLedMs = 900;
uint32_t featureLedUntil = 0;
bool featureLedActive = false;

bool validLed(uint8_t index) {
  return index < (sizeof(ledPins) / sizeof(ledPins[0])) && ledPins[index] >= 0;
}

void writeLed(uint8_t index, bool state) {
  if (validLed(index)) {
    digitalWrite(ledPins[index], state ? HIGH : LOW);
  }
}

void clearLeds() {
  for (uint8_t i = 0; i < sizeof(ledPins) / sizeof(ledPins[0]); ++i) {
    writeLed(i, false);
  }
}

}  // namespace

void ledBegin() {
  for (int pin : ledPins) {
    if (pin >= 0) {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
    }
  }
}

void ledSet(uint8_t index, bool state) {
  writeLed(index, state);
}

void ledSignalFeature(uint8_t index) {
  clearLeds();
  ledSet(index, true);
  featureLedActive = true;
  featureLedUntil = millis() + kFeatureLedMs;
}

void ledHeartbeat() {
  if (!featureLedActive) {
    return;
  }

  if (static_cast<int32_t>(millis() - featureLedUntil) >= 0) {
    clearLeds();
    featureLedActive = false;
  }
}

void updateyellow(bool state) {
  ledSet(0, state);
}

void updategreen(bool state) {
  ledSet(1, state);
}

void updateblue(bool state) {
  ledSet(2, state);
}
