#include <Arduino.h>

#include "include/battery.h"
#include "include/gps.h"
#include "include/input.h"
#include "include/ir_tools.h"
#include "include/led.h"
#include "include/nrf.h"
#include "include/pins.h"
#include "include/screen.h"
#include "include/wifi_tools.h"

void setup() {
  Serial.begin(115200);
  delay(50);

  inputBegin();
  irToolsBegin();
  ledBegin();
  batteryBegin();
  gpsBegin();
  nrfBegin();
  wifiToolsBegin();

  screenBegin();
  screenShowBoot();

  Serial.println(screenBuildName());
  Serial.println("RF tab includes safe nRF diagnostics, WiFi scan, and demo WiFi names.");
}

void loop() {
  updateGPS();
  irToolUpdate();
  wifiScanUpdate();
  wifiNameUpdate();
  nrfJammerUpdate(); 
  ledHeartbeat();

  const InputEvent event = inputUpdate();
  screenUpdate(event);
}
