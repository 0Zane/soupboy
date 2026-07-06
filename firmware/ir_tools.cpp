#include "include/ir_tools.h"

#include "include/pins.h"

namespace {

IRToolState state = IRToolState::Idle;
bool hasCapture = false;

}  // namespace

void irToolsBegin() {
  if (PIN_IR_OUT >= 0) {
    pinMode(PIN_IR_OUT, INPUT);
  }
}

void irToolActivate() {
  if (state == IRToolState::Idle) {
    state = IRToolState::Reading;
    return;
  }

  if (state == IRToolState::Reading) {
    hasCapture = true;
    state = IRToolState::Captured;
    return;
  }

  hasCapture = false;
  state = IRToolState::Idle;
}

void irToolUpdate() {
}

IRToolState irToolState() {
  return state;
}

bool irToolHasCapture() {
  return hasCapture;
}

const char *irToolStateText() {
  if (state == IRToolState::Reading) {
    return "reading";
  }
  if (state == IRToolState::Captured) {
    return "copy saved";
  }
  return "idle";
}
