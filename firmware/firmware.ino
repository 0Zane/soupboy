#include <Arduino.h>

#include "include/battery.h"
#include "include/gps.h"
#include "include/input.h"
#include "include/led.h"
#include "include/nrf.h"
#include "include/pins.h"
#include "include/screen.h"
#include "include/wifi_tools.h"

void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(PIN_SIGNAL_OUT, OUTPUT);
  digitalWrite(PIN_SIGNAL_OUT, LOW);

  inputBegin();
  ledBegin();
  batteryBegin();
  gpsBegin();
  nrfBegin();
  wifiToolsBegin();

  screenBegin();
  screenShowBoot();

  Serial.println(screenBuildName());
  Serial.println("RF/BLE TX tools disabled. Passive WiFi scan only.");
}

void loop() {
  updateGPS();
  wifiScanUpdate();
  ledHeartbeat();

  const InputEvent event = inputUpdate();
  screenUpdate(event);
}
