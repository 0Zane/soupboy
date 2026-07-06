#pragma once

#include <Arduino.h>

enum class IRToolState : uint8_t {
  Idle,
  Reading,
  Captured,
};

void irToolsBegin();
void irToolActivate();
void irToolUpdate();
IRToolState irToolState();
bool irToolHasCapture();
const char *irToolStateText();
