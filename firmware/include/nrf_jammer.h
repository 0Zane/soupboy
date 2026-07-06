#pragma once
#include <Arduino.h>

// Jammer modes
enum JamMode : uint8_t {
    JAM_BLE_21,      // 21 BLE channels
    JAM_BLE_80,      // 80 channels (full 2.4GHz sweep)
    JAM_WIFI,        // WiFi channels 1-14
    JAM_ZIGBEE,      // Zigbee channels
    JAM_DRONE,       // Drone frequencies
    JAM_OFF
};

// Status struct
struct JammerStatus {
    bool initialized;
    bool running;
    JamMode mode;
    uint8_t channel;
    uint32_t packetsSent;
    const char* message;
};

// API
void jammerBegin();
void jammerStart(JamMode mode);
void jammerStop();
void jammerToggle();
void jammerUpdate();  // call in loop
JammerStatus jammerStatus();
const char* jamModeName(JamMode mode);