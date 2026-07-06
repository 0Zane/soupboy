#pragma once

#include <Arduino.h>

void ledBegin();
void ledSet(uint8_t index, bool state);
void ledHeartbeat();

// Compatibility names from the first prototype.
void updateyellow(bool state);
void updategreen(bool state);
void updateblue(bool state);
