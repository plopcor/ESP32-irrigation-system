#include "lcdgfx.h"
#include "Keys.h"
#include "State.h"
#include <Preferences.h>
#include "Context.h"

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

#define IDLE_TIME 10 // Seconds till sleep when idle


DisplaySSD1306_128x64_I2C display(
  // ¿?, {busId, addr, scl, sda, freq}
  -1, { -1, DISPLAY_ADDR, DISPLAY_SCL_PIN, DISPLAY_SDA_PIN, 0}  
);

// App context and data
AppContext ctx;

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

  ctx.state.current = STATE::SET_TIMER;
  ctx.display.hasUpdate = true;
}

void loop() {
  updateKeys();

  doStateActions();

  // Update screen if needed
  if (ctx.display.hasUpdate) {
    updateScreen();
    ctx.display.hasUpdate = false;
    ctx.state.last = ctx.state.current;
  }

  delay(10);
}

// Handle keys
void keyPressed(Key keyPressed) {

  switch(ctx.state.current) {
    case STATE::SET_TIMER:
      updateTimerState(keyPressed);
      break;
    case STATE::WORKING:
      updateWorkingState(keyPressed);
      break;
  }

}

void updateTimerState(Key key) {
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
      // TODO: Go to deep sleep
      doDeepSleep();
      return;
      break;
  }

}

void updateWorkingState(Key key) {
  switch(key) {
    case Key::BACK:
      ctx.state.current = STATE::SET_TIMER;
      ctx.display.hasUpdate = true;
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
      ctx.state.current = STATE::SET_TIMER;
      ctx.display.hasUpdate = true;
    }
    
    //return;
  }

  // IDLE check
  if (ctx.state.current == STATE::SET_TIMER) {

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
    case STATE::SET_TIMER:
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
      display.printFixedN(0, ctx.display.fontHeight * 2, byteToFixedChars(ctx.irrigation.minutes), STYLE_BOLD, 1);

      // Calculate Liters (use L/min from pump store specifications)
      display.setTextCursor(0, ctx.display.fontHeight * 6);
      display.write(uintToStr( (unsigned int)ctx.irrigation.minutes * 3));
      display.write("L  ");
      // display.printFixed(0, 24, "Liters: ", STYLE_BOLD);
      // display.printFixed(10, 25, uintToStr( (unsigned int)minutes * 3), STYLE_BOLD);

      Serial.print("[Info] Timer (min): ");
      Serial.println(ctx.irrigation.minutes);

      break;
    case STATE::WORKING:

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
      break;
    // case STATE::FINISHED:
    //   if (stateChanged)
    //     Serial.println("[SCREEN] FINISHED");
    //   break;
  }

}

void setMosfet(bool active) {
  digitalWrite(MOSFET_PIN, active ? HIGH : LOW);
}

// --------------------------------

char* uintToStr(unsigned int value) {
  static char buffer[12]; // CAUTION: Reused between calls, not thread-safe
  snprintf(buffer, sizeof(buffer), "%u", value);
  return buffer;
}

char* byteToFixedChars(unsigned int value) {
  static char buffer[4]; // CAUTION: Reused between calls, not thread-safe
  snprintf(buffer, sizeof(buffer), "%-3u", value); // Use "-3" to right-pad withspaces 
  return buffer;
}
char* byteToChar(unsigned int value) {
  // Enough for 3-digit byte (0 to 255) + null terminator
  static char buffer[4]; // CAUTION: Reused between calls, not thread-safe
  //snprintf(buffer, sizeof(buffer), "%u", value);
  snprintf(buffer, sizeof(buffer), "%-3u", value); // Use "-3" to right-pad withspaces 
  return buffer;
}

void setupDeepSleep(int pin) {
  
  // Validate wakeup pin
  bool validPin = true;
  #if defined(CONFIG_IDF_TARGET_ESP32)
    // GPIOs 0, 2, 4, 12-15, 25-27, 32-39
    validPin = (pin == 0 || pin == 2 || pin == 4 || (pin >= 12 && pin <= 15) || (pin >= 25 && pin <= 27) || (pin >= 32 && pin <= 39));
  #elif defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
    validPin = (pin >= 0 && pin <= 21); // GPIOs 0-21
  #elif defined(CONFIG_IDF_TARGET_ESP32C3)
    validPin = (pin >= 0 && pin <= 5); // GPIOs 0-5
  #endif

  if (! validPin) {
    Serial.println("[ERROR] Wakeup pin is not RTC GPIO");
    while(true);
  }

  // Configure deep sleep wakeup pin
  #if defined(CONFIG_IDF_TARGET_ESP32C3)

    esp_err_t res = esp_deep_sleep_enable_gpio_wakeup(1 << BUTTON_SELECT_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);

    // NOTE: ESP32-C3 does not support RTC wakeup @ https://github.com/espressif/arduino-esp32/issues/7005#issuecomment-1188708571
    
    // DOCS @ https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/sleep_modes.html#_CPPv433esp_deep_sleep_enable_gpio_wakeup8uint64_t33esp_deepsleep_gpio_wake_up_mode_t
    
  #else

    esp_err_t res = esp_sleep_enable_ext0_wakeup((gpio_num_t) pin, LOW);

    // NOTE: Not valid for RISC-V processors (like the ESP32-C3 mini)

    // DOCS @ https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32/api-reference/system/sleep_modes.html#_CPPv428esp_sleep_enable_ext0_wakeup10gpio_num_ti

  #endif

  // Verify wakeup set
  if (res == ESP_ERR_INVALID_ARG) {
    Serial.println(F("[ERROR] 'BUTTON_SELECT_PIN' pin is not an RTC GPIO (for deep sleep)"));
    while(true);
  } else if (res == ESP_ERR_INVALID_STATE) {
    Serial.println(F("[ERROR] Wakeup trigger conflict"));
    while(true);
  } // else ESP_OK

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


