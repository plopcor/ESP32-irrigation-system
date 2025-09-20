#include <sys/_stdint.h>
#include "State.h"
#include "lcdgfx_gui.h"

#pragma once

struct StateContext {
  STATE current = STATE::INIT;
  STATE last = STATE::INIT;
};

struct IrrigationContext {
  uint8_t minutes;          // Timer set
  uint16_t count;           // Timer in action
  uint16_t totalCount;      // Timer in action
};

struct ScheduleContext {
  bool enabled;
  uint8_t frequency;        // Hours (0-255)
  uint8_t duration;         // Minutes (0-255)
  uint32_t nextMillis;
};

struct DisplayContext {
  bool hasUpdate;           // LCD updates
  uint32_t idleCount;       // IDLE
  uint8_t fontHeight = 8;
};


// Main context
struct AppContext {
  StateContext state;
  IrrigationContext irrigation;
  ScheduleContext schedule;
  DisplayContext display;
  uint32_t millis;          // Action delays without blocking
};

// Declare it as external so instance can be used from other cpp files
extern AppContext ctx;

extern LcdGfxMenu menu;
