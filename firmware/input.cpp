#include "include/input.h"

#include "include/pins.h"

namespace {

struct ButtonState {
  int pin;
  InputEvent tapEvent;
  InputEvent holdEvent;
  bool lastReading;
  bool stableState;
  bool holdSent;
  uint32_t changedAt;
  uint32_t pressedAt;
};

bool readButton(int pin) {
  const bool rawHigh = digitalRead(pin) == HIGH;
  return BUTTON_ACTIVE_HIGH ? rawHigh : !rawHigh;
}

ButtonState buttons[] = {
  {PIN_BUTTON_PREV, InputEvent::Previous, InputEvent::None, false, false, false, 0, 0},
  {PIN_BUTTON_NEXT, InputEvent::Next, InputEvent::None, false, false, false, 0, 0},
  {PIN_BUTTON_SELECT, InputEvent::Select, InputEvent::Back, false, false, false, 0, 0},
};

InputEvent updateButton(ButtonState &button, uint32_t now) {
  const bool reading = readButton(button.pin);

  if (reading != button.lastReading) {
    button.lastReading = reading;
    button.changedAt = now;
  }

  if ((now - button.changedAt) >= BUTTON_DEBOUNCE_MS &&
      reading != button.stableState) {
    button.stableState = reading;
    if (button.stableState) {
      button.pressedAt = now;
      button.holdSent = false;
    } else if (!button.holdSent) {
      return button.tapEvent;
    }
  }

  if (button.stableState && !button.holdSent &&
      button.holdEvent != InputEvent::None &&
      (now - button.pressedAt) >= BUTTON_LONG_PRESS_MS) {
    button.holdSent = true;
    return button.holdEvent;
  }

  return InputEvent::None;
}

}  // namespace

void inputBegin() {
  const int mode = BUTTON_ACTIVE_HIGH ? INPUT_PULLDOWN : INPUT_PULLUP;
  const uint32_t now = millis();

  for (ButtonState &button : buttons) {
    pinMode(button.pin, mode);
    button.lastReading = readButton(button.pin);
    button.stableState = button.lastReading;
    button.changedAt = now;
    button.pressedAt = button.stableState ? now : 0;
    button.holdSent = false;
  }
}

InputEvent inputUpdate() {
  const uint32_t now = millis();
  for (ButtonState &button : buttons) {
    const InputEvent event = updateButton(button, now);
    if (event != InputEvent::None) {
      return event;
    }
  }
  return InputEvent::None;
}
