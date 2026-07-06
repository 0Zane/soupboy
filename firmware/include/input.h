#pragma once

#include <Arduino.h>

enum class InputEvent : uint8_t {
  None,
  Previous,
  Next,
  Left,
  Right,
  Select,
  Back,
};

void inputBegin();
InputEvent inputUpdate();
