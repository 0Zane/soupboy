#include "include/ir_tools.h"

#include "include/pins.h"

namespace {

constexpr uint32_t kReadTimeoutMs = 3000;
constexpr uint8_t kCaptureEdgeThreshold = 12;

IRToolState state = IRToolState::Idle;
bool hasCapture = false;
uint32_t readStartAt = 0;
uint8_t edgeCount = 0;
int lastLevel = LOW;

void startReading() {
  hasCapture = false;
  edgeCount = 0;
  readStartAt = millis();
  lastLevel = PIN_IR_OUT >= 0 ? digitalRead(PIN_IR_OUT) : LOW;
  state = IRToolState::Reading;
  Serial.println("IR reading started");
}

}  // namespace

void irToolsBegin() {
  if (PIN_IR_OUT < 0) {
    Serial.println("IR receiver disabled: pin not configured");
    return;
  }

  pinMode(PIN_IR_OUT, INPUT);
  lastLevel = digitalRead(PIN_IR_OUT);
  Serial.println("IR receiver pin ready");
}

void irToolActivate() {
  if (state == IRToolState::Idle || state == IRToolState::Captured) {
    startReading();
    return;
  }

  if (state == IRToolState::Reading) {
    state = hasCapture ? IRToolState::Captured : IRToolState::Idle;
  }
}

void irToolUpdate() {
  if (state != IRToolState::Reading || PIN_IR_OUT < 0) {
    return;
  }

  const uint32_t now = millis();
  const int level = digitalRead(PIN_IR_OUT);
  if (level != lastLevel) {
    lastLevel = level;
    if (edgeCount < UINT8_MAX) {
      ++edgeCount;
    }
  }

  if (edgeCount >= kCaptureEdgeThreshold) {
    hasCapture = true;
    state = IRToolState::Captured;
    Serial.printf("IR activity captured: %u edges\n", edgeCount);
    return;
  }

  if (now - readStartAt > kReadTimeoutMs) {
    state = IRToolState::Idle;
    hasCapture = false;
    Serial.println("IR reading timed out");
  }
}

IRToolState irToolState() {
  return state;
}

bool irToolHasCapture() {
  return hasCapture;
}

const char *irToolStateText() {
  switch (state) {
    case IRToolState::Idle: return "idle";
    case IRToolState::Reading: return "reading";
    case IRToolState::Captured: return "signal saved";
  }
  return "unknown";
}
