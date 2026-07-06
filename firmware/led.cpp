#include "include/led.h"

#include "include/pins.h"

namespace {

constexpr int ledPins[] = {PIN_LED1, PIN_LED2, PIN_LED3};
uint32_t lastHeartbeatAt = 0;
bool heartbeatState = false;

bool validLed(uint8_t index) {
  return index < (sizeof(ledPins) / sizeof(ledPins[0])) && ledPins[index] >= 0;
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
  if (validLed(index)) {
    digitalWrite(ledPins[index], state ? HIGH : LOW);
  }
}

void ledHeartbeat() {
  const uint32_t now = millis();
  if (now - lastHeartbeatAt < 900) {
    return;
  }

  lastHeartbeatAt = now;
  heartbeatState = !heartbeatState;
  ledSet(0, heartbeatState);
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
