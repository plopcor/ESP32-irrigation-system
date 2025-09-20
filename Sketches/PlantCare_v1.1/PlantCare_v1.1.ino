#include "lcdgfx.h"
#include "Keys.h"
#include "State.h"
#include <Preferences.h>
#include "Context.h"
#include "lcdgfx_gui.h"
#include "Utils.h"

#define DISPLAY_ADDR 0x3C
#define DISPLAY_SCL_PIN 6 // ESP32-Dev: 22, ESP32-C3: 9 or 6
#define DISPLAY_SDA_PIN 7 // ESP32-Dev: 21, ESP32-C3: 8 or 7

// ESP32-Dev: 19, 18, 15, 17
// ESP32-C3: 0, 1, 3, 4
#define BUTTON_UP_PIN 0
#define BUTTON_DOWN_PIN 1
#define BUTTON_SELECT_PIN 3 // Needs RTC GPIO for deep sleep
#define BUTTON_BACK_PIN 4

#define MOSFET_PIN 8 // ESP32-Dev LED: 2, ESP32-C3 LED: 8

#define IDLE_TIME 10     // Seconds till sleep when idle
#define PUMP_L_PER_MIN 3  // Pump liter per minute

DisplaySSD1306_128x64_I2C display(
  // ¿?, {busId, addr, scl, sda, freq}
  -1, { -1, DISPLAY_ADDR, DISPLAY_SCL_PIN, DISPLAY_SDA_PIN, 0}  
);

// App context and data
AppContext ctx;

// Define menu
// const char *menuItems[] =
// {
//   "MANUAL",   // Manual irrigation
//   "SETUP",    // Setup schedule
//   "CHECK SETUP", // Check schedule
//   "SKIP SETUP",      // Skip next irrigation/schedule
//   "DISABLE SETUP",    // Disable schedule
// };

const char *menuItems[] =
{
  "MANUAL",       // Manual irrigation
  "SETUP",        // Setup schedule
  "SETUP>CHECK",  // Check schedule
  "SETUP>SKIP",   // Skip next irrigation/schedule
  "SETUP>DISABLE",// Disable schedule
};

LcdGfxMenu menu( menuItems, sizeof(menuItems) / sizeof(char *) );
// LcdGfxMenu menu( menuItems, sizeof(menuItems) / sizeof(char *), (NanoRect){{-4,0}, {0,0} } );

// getFont() @ https://codedocs.xyz/lexus2k/lcdgfx/class_nano_display_base.html#a7a95824cfe5976a05b8c44390a8c72bf
// getHeader() @ https://codedocs.xyz/lexus2k/lcdgfx/class_nano_font.html#a827aff2a9659a43607a6cd09c03ec27d
// height @ https://codedocs.xyz/lexus2k/lcdgfx/struct_s_font_header_record.html
// display<>.getFont()<NanoFont>.getHeader()<SFontHeaderRecord>.height<uint8_t>


// // Memory storage
// Preferences preferences;

void setup() {

  Serial.begin(115200);

  // // Access storage
  // preferences.begin("plantcare", false); // Open
  // // minutes = preferences.getUInt("minutes", 0);       // Retrieve
  // minutes = preferences.getBytes("timer", minutes, 1);       // Retrieve

  // Set deep sleep GPIO trigger
  setupDeepSleep(BUTTON_SELECT_PIN);

  // Map keys to button GPIO pins
  setupKeys(
    BUTTON_UP_PIN,
    BUTTON_DOWN_PIN,
    BUTTON_SELECT_PIN,
    BUTTON_BACK_PIN
  );
  setupKeyCallback(keyPressed);

  // MOSFET pin
  pinMode(MOSFET_PIN, OUTPUT);

  // Select the font to use with menu and all font functions
  //display.setFixedFont( ssd1306xled_font6x8 );
  display.setFixedFont( ssd1306xled_font8x16 );
  display.begin();
  display.clear();
  ctx.display.fontHeight = display.getFont().getHeader().height / 2;

  // menu.show(display);
  ctx.state.current = STATE::MENU;
  ctx.display.hasUpdate = true;
}

void loop() {
  updateKeys();

  doStateActions();

  // Update screen if needed
  if (ctx.display.hasUpdate) {
    updateScreen();
    ctx.display.hasUpdate = false;
    // Serial.println("============== STATE =========");
    // Serial.print((int)ctx.state.last);
    // Serial.print(" -> ");
    // Serial.println((int)ctx.state.current);
    ctx.state.last = ctx.state.current;
  }

  delay(10);
}

// Handle keys
void keyPressed(Key keyPressed) {

  switch(ctx.state.current) {
    case STATE::MENU:
      updateMenuState(keyPressed);
      break;
    case STATE::SET_TIMER:
      updateTimerState(keyPressed);
      break;
    case STATE::WORKING:
      updateWorkingState(keyPressed);
      break;
    case STATE::SCHEDULE_SET_START:
      updateScheduleStateSetStart(keyPressed);
      break;
    case STATE::SCHEDULE_SET_FREQ:
      updateScheduleStateSetFrequency(keyPressed);
      break;
    case STATE::SCHEDULE_SET_DURATION:
      updateScheduleStateSetDuration(keyPressed);
      break;
    case STATE::SCHEDULE_CHECK:
      updateScheduleStateCheck(keyPressed);
      break;
  }

}

// Main loop/actions update
void doStateActions() {

  if (ctx.state.current == STATE::WORKING) {

    // Count
    if (ctx.irrigation.count < ctx.irrigation.totalCount) {

      // ===== METHOD 1: Has "drift"
      /*
      currentMilis = millis();
      if (currentMilis - previousMilis >= 1000) {
        previousMilis = currentMilis
        count++;
      }
      */

      // ==== METHOD 2
      if (millis() - ctx.millis >= 1000) {
        setMosfet(true);
        ctx.millis += 1000;
        ctx.irrigation.count++;
        ctx.display.hasUpdate = true;
      }

    } else {
      // Finish count
      Serial.println("[Info] Count: FINISHED");
      setMosfet(false);
      ctx.state.current = STATE::MENU;
      ctx.display.hasUpdate = true;
    }
    
    //return;
  }

  // IDLE check

  if (ctx.state.current != STATE::WORKING) {

  // if (ctx.state.current == STATE::MENU ||
  //     ctx.state.current == STATE::SET_TIMER) {

    // Ignore when
    // - Navigating to SET_TIME (state changed to SET_TIMER)
    // - Input is sent (hasUpdate true)
    if (ctx.state.last != ctx.state.current || ctx.display.hasUpdate) {

      ctx.display.idleCount = 0;
      return;

    } else {

      // Check if idle (no input) and deep sleep
      if (millis() - ctx.millis >= 1000) {
        ctx.millis += 1000;
        ctx.display.idleCount++;
        if (ctx.display.idleCount >= IDLE_TIME)
          doDeepSleep();
        return;
      }

    }
  }

}

// Screen updates
void updateScreen() {

  bool stateChanged = ctx.state.last != ctx.state.current;

  if (stateChanged)
    display.clear();

  switch (ctx.state.current) {

    case STATE::MENU:
      Serial.println("[MENU] SHOW");
      menu.show(display);
      break; 

    case STATE::SET_TIMER:
    {
      // LCD write "Timer"

      if (stateChanged) {
        Serial.println("[SCREEN] SET_TIMER");
        // display.setTextCursor(0, 0);
        // display.write("TIMER (min)");
        // display.write("Hello world hehe");
        // display.setTextCursor(0, 2);
        // display.write("XX");
        display.printFixed(0, 0, "TIMER (min)");
      }

      // Print minutes (x2 fontsize)
      display.printFixedN(0, ctx.display.fontHeight * 2, uint8ToChars(ctx.irrigation.minutes, true), STYLE_BOLD, 1);

      // Calculate Liters (use L/min from pump store specifications)
      display.setTextCursor(0, ctx.display.fontHeight * 6);
      display.write(uint16ToChars( (uint16_t)ctx.irrigation.minutes * PUMP_L_PER_MIN));
      display.write("L  ");
      // display.printFixed(0, 24, "Liters: ", STYLE_BOLD);
      // display.printFixed(10, 25, uint16ToStr( (uint16_t)minutes * 3), STYLE_BOLD);

      Serial.print("[Info] Timer (min): ");
      Serial.println(ctx.irrigation.minutes);
    }
      break;
    case STATE::WORKING:
    {
      Serial.println("============ WORKING 3");
      if (stateChanged)
        Serial.println("[SCREEN] WORKING");
      Serial.print("[Info] Count: ");
      Serial.println(ctx.irrigation.count);

      if (stateChanged)
        display.printFixed(0, 0, "Watering", STYLE_NORMAL);

      int percent = map(
        ctx.irrigation.count,
        0, ctx.irrigation.totalCount,
        0, 100
      );
      display.drawProgressBar(percent);
    }
    break;

    case STATE::SCHEDULE_SET_START:
      if (stateChanged)
        display.printFixed(0, 0, "Cycle start", STYLE_NORMAL);

      display.setTextCursor(0, ctx.display.fontHeight * 2);
      display.write("From now: ");
      display.write(int8ToChars(startDiff));
      display.write("h");
    break;
    case STATE::SCHEDULE_SET_FREQ:
      if (stateChanged)
        display.printFixed(0, 0, "Frequency", STYLE_NORMAL);

      // TODO: 0-168 (0h to 1 week)
      display.setTextCursor(0, ctx.display.fontHeight * 2);
      display.write("Hours: ");
      display.write(uint8ToChars(ctx.schedule.frequency));
    break;
    case STATE::SCHEDULE_SET_DURATION:
      if (stateChanged)
        display.printFixed(0, 0, "Duration", STYLE_NORMAL);

      display.setTextCursor(0, ctx.display.fontHeight * 2);
      display.write("Minutes: ");
      display.write(uint8ToChars(ctx.schedule.duration));
    break;

    case STATE::SCHEDULE_CHECK:
    {
      if (stateChanged)
        display.printFixed(0, 0, "Next in", STYLE_NORMAL);

      display.setTextCursor(0, ctx.display.fontHeight * 2);
      if (ctx.schedule.enabled) {

        uint32_t currentMillis = ctx.schedule.nextMillis - millis();
        uint8_t hours = currentMillis / 3600000UL;
        uint8_t minutes = (currentMillis % 3600000UL) / 60000UL;

        display.write(uint8ToChars(hours));
        display.write("h ");
        display.write(uint8ToChars(minutes));
        display.write("min");

        display.setTextCursor(0, ctx.display.fontHeight * 4);
        display.write("For ");
        display.write(uint8ToChars(ctx.schedule.duration));
        display.write("min (");
        uint16_t liters = ctx.schedule.duration * PUMP_L_PER_MIN;
        display.write(uint16ToChars(liters));
        display.write("L)");
      } else {
        display.write("[Disabled]");
      }
      break;
    }
  }

}

void setMosfet(bool active) {
  digitalWrite(MOSFET_PIN, active ? HIGH : LOW);
}

void doDeepSleep() {
  Serial.println(">>> DEEP SLEEP");
  // display.clear();
  display.end();
  display.getInterface().displayOff();
  esp_deep_sleep_start();
}


/*
void goDeepSleep() {

  // ESP32-C3 does not support RTC wakeup @ https://github.com/espressif/arduino-esp32/issues/7005#issuecomment-1188708571
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    esp_err_t res = esp_deep_sleep_enable_gpio_wakeup(1 << BUTTON_SELECT_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
    
  #else
    // 
  #endif

  if (res == ESP_ERR_INVALID_ARG) {
    Serial.println(F("[ERROR] 'BUTTON_SELECT_PIN' pin is not an RTC GPIO (for deep sleep)"));
  } else if (res == ESP_ERR_INVALID_STATE) {
    Serial.println(F("[ERROR] Wakeup trigger conflict"));
  }

  esp_deep_sleep_start();

  // #if defined(SOC_PM_SUPPORT_EXT_WAKEUP) // SOC_PM_SUPPORT_EXT0_WAKEUP
  // #else
  // #endif
}
*/
