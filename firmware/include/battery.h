#pragma once

#include <Arduino.h>

void batteryBegin();
float batteryVoltage();
int batteryPercent();
const char *batteryStatusText();
