
#include "State.h"

#pragma once

struct StateContext {
  STATE current = STATE::INIT;
  STATE last = STATE::INIT;
};

struct IrrigationContext {
  byte minutes;             // Timer set
  unsigned int count;       // Timer in action
  unsigned int totalCount;  // Timer in action
};

// struct Schedule {}

struct DisplayContext {
  bool hasUpdate;           // LCD updates
  unsigned long idleCount;  // IDLE
  uint8_t fontHeight = 8;
};


// Main context
struct AppContext {
  StateContext state;
  IrrigationContext irrigation;
  DisplayContext display;
  unsigned long millis;     // Action delays without blocking
};
