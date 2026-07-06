#pragma once

#include <Arduino.h>

enum class InputEvent : uint8_t {
  None,
  Left,
  Right,
  Select,
  Home,
};

void inputBegin();
InputEvent inputUpdate();
