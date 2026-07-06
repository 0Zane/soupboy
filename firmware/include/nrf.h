#pragma once

#include <Arduino.h>

struct NrfStatus {
  bool pinsConfigured;
  bool moduleDetected;
  bool txEnabled;
  const char *message;
};

void nrfBegin();
NrfStatus nrfStatus();
const char *nrfSafeModeText();
