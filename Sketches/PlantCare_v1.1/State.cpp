#include "HardwareSerial.h"
#include <sys/_stdint.h>
# pragma once

#include "State.h"
#include "Keys.h"
#include "Context.h"
#include "Utils.h"

// Init
int8_t startDiff = 0;

// TODO: Map "Keys.h" function "setupKeyCallback()" to this function directly on "PlantCare.ino"
// void handleStateInput(STATE state, Key key)
// {

// }


void updateMenuState(Key key)
{
  switch (key) {
    case Key::UP:
      menu.up();
    break;
    case Key::DOWN:
      menu.down();
    break;
    case Key::SELECT:
    {
      // Request current position of menu cursor
      // menu.selection()
            
      // Menu actions
      switch (menu.selection()) {
        case 0: // MANUAL
          ctx.state.current = STATE::SET_TIMER;
          break;
        case 1: // SETUP
          ctx.state.current = STATE::SCHEDULE_SET_START;
          break;
        case 2: // SETUP>CHECK
          ctx.state.current = STATE::SCHEDULE_CHECK;
          break;
        case 3: // SETUP>SKIP
          // Add some millis OR trigger flag and next millis OR likely
          ctx.schedule.nextMillis += (ctx.schedule.frequency * 3600000); // H to ms
          /*
            TODO
            - Update "deep sleep wakeup" trigger
            - Limit "skip" to only one skip? Or can be used twice or more?
          */
          break;
        case 4: // SETUP>DISABLE
          ctx.schedule.enabled = false;
          /*
            TODO:
            - Disable "deep sleep wakeup" trigger
          */
          break;
      }

      ctx.display.hasUpdate = true;
    }
    break;
    case Key::BACK:
      doDeepSleep();
    break;
  }

  ctx.display.hasUpdate = true;

}

void updateTimerState(Key key)
{
  switch (key) {
    case Key::UP:
      if (ctx.irrigation.minutes < 255) {
        ctx.irrigation.minutes += 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::DOWN:
      if (ctx.irrigation.minutes > 0) {
        ctx.irrigation.minutes -= 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::SELECT:
      if (ctx.irrigation.minutes > 0) {
        //preferences.setBytes("timer", minutes, 1); // Store to memory
        ctx.irrigation.totalCount = ctx.irrigation.minutes * 60; // to seconds
        ctx.irrigation.count = 0;
        ctx.millis = millis();
        ctx.state.current = STATE::WORKING;
        ctx.display.hasUpdate = true;
        return;
      }
      break;
    case Key::BACK:
      ctx.state.current = STATE::MENU;
      ctx.display.hasUpdate = true;
      return;
      break;
  }

}

void updateWorkingState(Key key)
{
  switch(key) {
    case Key::BACK:
      ctx.state.current = STATE::MENU;
      ctx.display.hasUpdate = true;
    break;
  }
}

void updateScheduleStateSetStart(Key key)
{
  switch (key) {
    case Key::UP:
      if (startDiff < 23) {
        startDiff += 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::DOWN:
      if (startDiff > -23) {
        startDiff -= 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::SELECT:
      ctx.state.current = STATE::SCHEDULE_SET_FREQ;
      ctx.display.hasUpdate = true;
      return;
    case Key::BACK:
      ctx.state.current = STATE::MENU;
      ctx.display.hasUpdate = true;
      return;
  }

  // ctx.display.hasUpdate = true;
}

void updateScheduleStateSetFrequency(Key key)
{
  switch (key) {
    case Key::UP:
      if (ctx.schedule.frequency < 168) { // 7 days
        ctx.schedule.frequency += 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::DOWN:
      if (ctx.schedule.frequency > 0) {
        ctx.schedule.frequency -= 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::SELECT:
      if (ctx.schedule.frequency > 0) {
        ctx.state.current = STATE::SCHEDULE_SET_DURATION;
        ctx.display.hasUpdate = true;
      }
      return;
    case Key::BACK:
      ctx.state.current = STATE::SCHEDULE_SET_START;
      ctx.display.hasUpdate = true;
      return;
  }
}

void updateScheduleStateSetDuration(Key key)
{
  switch (key) {
    case Key::UP:
      if (ctx.schedule.duration < 255) { // 
        ctx.schedule.duration += 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::DOWN:
      if (ctx.schedule.duration > 0) {
        ctx.schedule.duration -= 1;
        ctx.display.hasUpdate = true;
      }
      break;
    case Key::SELECT:
      if (ctx.schedule.duration > 0) {

        ctx.schedule.enabled = true;

        if (startDiff > 0) {
          // Set current cycle
          ctx.schedule.nextMillis = millis() + (startDiff * 3600000); // h to ms (* 60 * 60 * 1000)
          // Today + hours to start later

        } else {
          // Set next cycle
          ctx.schedule.nextMillis = millis() - (startDiff * 3600000) + (ctx.schedule.frequency * 3600000);
          // current - diff + frequency = Today's cycle start time + next cycle time
        }

        ctx.state.current = STATE::MENU;
        ctx.display.hasUpdate = true;
      }
      return;
    case Key::BACK:
      ctx.state.current = STATE::SCHEDULE_SET_FREQ;
      ctx.display.hasUpdate = true;
      return;
  }
}

void updateScheduleStateCheck(Key key)
{
  switch (key) {
    case Key::UP:
    case Key::DOWN:
      return;
      // break;
    case Key::SELECT:
    case Key::BACK:
      ctx.state.current = STATE::MENU;
      ctx.display.hasUpdate = true;
      return;
  }
}