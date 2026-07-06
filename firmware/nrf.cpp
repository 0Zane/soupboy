#include "include/nrf.h"
#include "include/nrf_jammer.h"
#include "include/pins.h"

namespace {

NrfStatus status = {
  false,
  false,
  false,
  "CE/CSN not mapped",
};

}  // namespace

void nrfBegin() {
  status.pinsConfigured = PIN_NRF_CE >= 0 && PIN_NRF_CSN >= 0 &&
                          PIN_NRF_MISO >= 0 && PIN_NRF_MOSI >= 0 &&
                          PIN_NRF_SCK >= 0;
  status.moduleDetected = false;
  status.txEnabled = false;
  status.message = status.pinsConfigured ? "driver not enabled" : "pins unclear";
}

NrfStatus nrfStatus() {
  return status;
}

const char *nrfSafeModeText() {
  return "TX disabled";
}

// Wrappers for jammer
void nrfJammerInit() { jammerBegin(); }
void nrfJammerStart(int mode) { jammerStart((JamMode)mode); }
void nrfJammerStop() { jammerStop(); }
void nrfJammerToggle() { jammerToggle(); }
void nrfJammerUpdate() { jammerUpdate(); }
int nrfJammerGetMode() { return (int)jammerStatus().mode; }
bool nrfJammerIsRunning() { return jammerStatus().running; }
uint32_t nrfJammerGetPackets() { return jammerStatus().packetsSent; }
int nrfJammerGetChannel() { return jammerStatus().channel; }
const char* nrfJammerGetModeName() { return jamModeName(jammerStatus().mode); }