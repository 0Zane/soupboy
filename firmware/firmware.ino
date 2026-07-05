//System libraries
#include <Arduino.h>
#include <Wire.h>

//Custom firmware headers
#include "include/gps.h"
#include "include/pins.h"
#include "include/temperature.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
    // I2C INITIALIZATION (for BME280 and PCF8574)
    Serial.println("Initializing I2C bus...");
    pinMode(TEMP_SDA, INPUT_PULLUP);
    pinMode(TEMP_SCL, INPUT_PULLUP);
    delay(100);
    Wire.begin(TEMP_SDA, TEMP_SCL);
    Wire.setClock(100000);  // Slow clock for reliability
    delay(200);
    Serial.println("I2C Bus initialized at 100kHz");
    
    // GPS INITIALIZATION
    Serial.println("Initializing GPS UART...");
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    delay(100);
    Serial.println("Serial 2 started at 9600 baud rate");
    
        Serial.println("Initializing BME280 temperature sensor (0x76)...");
    int bme_retries = 3;
    while (!bme.begin(0x76, &Wire) && bme_retries > 0) {
        Serial.print("BME280 init failed, retrying... (");
        Serial.print(bme_retries);
        Serial.println(" retries left)");
        recoverI2C();
        Wire.begin(TEMP_SDA, TEMP_SCL);
        Wire.setClock(100000);
        delay(500);
        bme_retries--;
    }
    
    if (bme_retries == 0) {
        Serial.println("ERROR: Could not initialize BME280 after retries!");
    } else {
        Serial.println("BME280 found!");
    }
    delay(100);

}

void loop() {
  Serial.println("Hello");
  delay(1000);
  


  // PROTOTYPING
  Serial.print("Temperature = ");
  Serial.print(readTemperature());
  Serial.println(" *C");
  
  Serial.print("Pressure = ");
  Serial.print(readPressure());
  Serial.println(" hPa");
  
  Serial.print("Humidity = ");
  Serial.print(readHumidity());
  Serial.println(" %");
  Serial.println();


  updateGPS();
  if (isGPSValid()) {
      double latitude = getLatitude();
      double longitude = getLongitude();
      double speed = getSpeed();
      double altitude = getAltitude();
      uint32_t satellites = getSatellites();
      // Now you can use and store these values
      Serial.print("LAT: "); Serial.println(latitude, 6);
      Serial.print("LONG: "); Serial.println(longitude, 6);
      Serial.print("SPEED (km/h): "); Serial.println(speed);
      Serial.print("ALT (m): "); Serial.println(altitude);
      Serial.print("Satellites: "); Serial.println(satellites);
      Serial.print("Time: "); Serial.println(getDateTime());
  } else {
      Serial.println(">> Waiting for GPS lock (satellite acquisition)...");
  }

}

// I2C Bus recovery function
void recoverI2C() {
    Serial.println("Attempting I2C bus recovery...");
    pinMode(TEMP_SDA, INPUT_PULLUP);
    pinMode(TEMP_SCL, INPUT_PULLUP);
    delay(100);
    for (int i = 0; i < 10; i++) {
        digitalWrite(TEMP_SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(TEMP_SCL, LOW);
        delayMicroseconds(5);
    }
    pinMode(TEMP_SDA, INPUT);
    pinMode(TEMP_SCL, INPUT);
    delay(100);
    Serial.println("I2C recovery complete");
}