#pragma once
#include "nrf_jammer.h"
#include <Arduino.h>
// New jammer functions
void nrfJammerInit();
void nrfJammerStart(int mode);
void nrfJammerStop();
void nrfJammerToggle();
void nrfJammerUpdate();
int nrfJammerGetMode();
bool nrfJammerIsRunning();
uint32_t nrfJammerGetPackets();
int nrfJammerGetChannel();
const char* nrfJammerGetModeName();

struct NrfStatus {
  bool pinsConfigured;
  bool moduleDetected;
  bool txEnabled;
  const char *message;
};

void nrfBegin();
NrfStatus nrfStatus();
const char *nrfSafeModeText();
