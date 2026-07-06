#include "include/gps.h"
#include "include/pins.h"        // defines PIN_GPS_RX, PIN_GPS_TX, PIN_GPS_PPS, GPS_BAUD
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// The TinyGPS++ object
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// PPS tracking
volatile bool gpsPPSDetected = false;
volatile uint32_t gpsPPSMillis = 0;

void gpsBegin() {
  // Start UART with RX=16, TX=17
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  gpsSerial.setTimeout(10);

  // Configure PPS pin and attach interrupt
  if (PIN_GPS_PPS >= 0) {
    pinMode(PIN_GPS_PPS, INPUT_PULLUP);           // or INPUT if external pull-up
    attachInterrupt(digitalPinToInterrupt(PIN_GPS_PPS), gpsPPSInterrupt, RISING);
  }
}

// ISR for PPS – must be declared with IRAM_ATTR
void IRAM_ATTR gpsPPSInterrupt() {
  gpsPPSDetected = true;
  gpsPPSMillis = millis();
}

// Update GPS data from serial
void updateGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

// ---- PPS query functions ----
bool gpsHasPPS() {
  bool state = gpsPPSDetected;
  gpsPPSDetected = false;   // clear after read
  return state;
}

uint32_t gpsLastPPSMillis() {
  return gpsPPSMillis;
}

// ---- Existing getters ----
bool isGPSValid() {
  return gps.location.isValid();
}

uint32_t getSatellites() {
  return gps.satellites.value();
}

double getHDOP() {
  return gps.hdop.value() / 100.0;
}

double getLatitude() {
  return gps.location.lat();
}

double getLongitude() {
  return gps.location.lng();
}

double getSpeed() {
  return gps.speed.kmph();
}

double getAltitude() {
  return gps.altitude.meters();
}

String getDateTime() {
  if (!gps.date.isValid() || !gps.time.isValid()) {
    return "--";
  }
  return String(gps.date.year()) + "/" + String(gps.date.month()) + "/" + String(gps.date.day()) + "," + 
         String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
}

uint32_t gpsCharsProcessed() {
  return gps.charsProcessed();
}