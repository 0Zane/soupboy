#include "include/gps.h"

// The TinyGPS++ object
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

void gpsBegin() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  gpsSerial.setTimeout(10);

  if (PIN_GPS_PPS >= 0) {
    pinMode(PIN_GPS_PPS, INPUT);
  }
}

// Update GPS data from serial
void updateGPS() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
}

// Check if GPS has a valid fix
bool isGPSValid() {
  return gps.location.isValid();
}

// Get number of satellites
uint32_t getSatellites() {
  return gps.satellites.value();
}

// Get HDOP value (Horizontal Dilution of Precision)
double getHDOP() {
  return gps.hdop.value() / 100.0;
}

// Get latitude
double getLatitude() {
  return gps.location.lat();
}

// Get longitude
double getLongitude() {
  return gps.location.lng();
}

// Get speed in km/h
double getSpeed() {
  return gps.speed.kmph();
}

// Get altitude in meters
double getAltitude() {
  return gps.altitude.meters();
}

// Get date and time as string
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
