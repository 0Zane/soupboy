#include "include/battery.h"

#include "include/pins.h"

namespace {

constexpr uint32_t kBatteryReadIntervalMs = 1000;
constexpr uint8_t kBatterySamples = 8;

float lastVoltage = 0.0f;
uint32_t lastReadAt = 0;

float readBatteryNow() {
  if (PIN_BATTERY_ADC < 0) {
    return 0.0f;
  }

  uint32_t millivolts = 0;
  for (uint8_t i = 0; i < kBatterySamples; ++i) {
#if defined(ARDUINO_ARCH_ESP32)
    millivolts += analogReadMilliVolts(PIN_BATTERY_ADC);
#else
    millivolts += map(analogRead(PIN_BATTERY_ADC), 0, 1023, 0, 3300);
#endif
    delayMicroseconds(250);
  }

  const float adcVolts = (millivolts / static_cast<float>(kBatterySamples)) / 1000.0f;
  return adcVolts * BATTERY_DIVIDER_RATIO;
}

}  // namespace

void batteryBegin() {
  if (PIN_BATTERY_ADC < 0) {
    return;
  }

#if defined(ARDUINO_ARCH_ESP32)
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
#endif
  lastVoltage = readBatteryNow();
  lastReadAt = millis();
}

float batteryVoltage() {
  const uint32_t now = millis();
  if (now - lastReadAt >= kBatteryReadIntervalMs) {
    lastVoltage = readBatteryNow();
    lastReadAt = now;
  }
  return lastVoltage;
}

int batteryPercent() {
  const float volts = batteryVoltage();
  if (volts < 0.2f) {
    return -1;
  }

  // Conservative 1S Li-ion estimate. The UI labels this as an estimate because
  // the schematic only exposes a raw divider and the final pack chemistry may vary.
  const float percent = (volts - 3.30f) * 100.0f / (4.18f - 3.30f);
  return constrain(static_cast<int>(percent + 0.5f), 0, 100);
}

const char *batteryStatusText() {
  const float volts = batteryVoltage();
  if (volts < 0.2f) {
    return "offline";
  }
  if (volts < 3.45f) {
    return "low";
  }
  if (volts > 4.25f) {
    return "cal?";
  }
  return "stable";
}
