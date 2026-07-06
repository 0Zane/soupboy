#include "include/nrf_jammer.h"

namespace {

JammerStatus status = {
  false,
  false,
  JAM_OFF,
  0,
  0,
  "TX disabled",
};

}  // namespace

void jammerBegin() {
  status.initialized = false;
  status.running = false;
  status.mode = JAM_OFF;
  status.message = "TX disabled";
}

void jammerStart(JamMode mode) {
  status.initialized = false;
  status.running = false;
  status.mode = mode;
  status.message = "TX disabled";
}

void jammerStop() {
  status.running = false;
  status.mode = JAM_OFF;
  status.message = "TX disabled";
}

void jammerToggle() {
  jammerStop();
}

void jammerUpdate() {
}

JammerStatus jammerStatus() {
  return status;
}

const char *jamModeName(JamMode mode) {
  switch (mode) {
    case JAM_BLE_21: return "BLE 21ch";
    case JAM_BLE_80: return "BLE 80ch";
    case JAM_WIFI: return "WiFi";
    case JAM_ZIGBEE: return "Zigbee";
    case JAM_DRONE: return "Drone";
    case JAM_OFF: return "OFF";
  }
  return "Unknown";
}
