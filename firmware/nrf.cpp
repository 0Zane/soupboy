#include "include/nrf.h"
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
