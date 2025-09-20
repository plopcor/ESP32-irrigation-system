#include <sys/_stdint.h>
#include "Keys.h"

#pragma once

enum class STATE: uint8_t
{
  INIT,
  MENU,
  SET_TIMER,
  SCHEDULE_SET, // DELETE
  SCHEDULE_SET_START,
  SCHEDULE_SET_FREQ,
  SCHEDULE_SET_DURATION,
  SCHEDULE_CHECK,
  WORKING,
  FINISHED,
  PROGRAM
};

extern int8_t startDiff; // Hours of difference to start schedule cycle

void updateMenuState(Key key);
void updateTimerState(Key key);
void updateWorkingState(Key key);
void updateScheduleStateSetStart(Key key);
void updateScheduleStateSetFrequency(Key key);
void updateScheduleStateSetDuration(Key key);
void updateScheduleStateCheck(Key key);

// void handleInput_Schedule_SetStart();
