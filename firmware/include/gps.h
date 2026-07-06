#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

#include "pins.h"

extern TinyGPSPlus gps;
extern HardwareSerial gpsSerial;

void gpsBegin();

// Update GPS data from serial.
void updateGPS();

// GPS data getters
bool isGPSValid();
uint32_t getSatellites();
double getHDOP();
double getLatitude();
double getLongitude();
double getSpeed();
double getAltitude();
String getDateTime();
uint32_t gpsCharsProcessed();
