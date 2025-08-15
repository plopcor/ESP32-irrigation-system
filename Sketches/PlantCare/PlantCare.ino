#include "lcdgfx.h"
#include "Keys.h"
#include "Menu.h"
//#include <Preferences.h>

// Display
DisplaySSD1306_128x64_I2C display(-1, { -1, 0x3C, 22, 21, 0}); // {busId, addr, scl, sda, freq}

// Mosfet
byte mosfetPin = 2;

// STATE AND VARIABLES
STATE currentState = STATE::SET_TIMER;
STATE lastState = STATE::INIT;
unsigned int minutes; // TODO: Get from memory OR 0
unsigned int count; // Countdown counter (in minutes)
byte animationKey = 0;
unsigned long lastMillis; // For action delays without blocking
bool hasUpdate = false; // For LCD updates


// Memory storage
// Preferences preferences;

void setup() {

  Serial.begin(115200);

  // Access storage
  // preferences.begin("plantcare", false); // Open
  // minutes = preferences.getUInt("minutes", 0);       // Retrieve

  // Map keys to button GPIO pins
  setupKeys(19, 18, 5, 17); // UP, DOWN, SELECT, BACK
  setupKeyCallback(keyPressed);

  // MOSFET pin
  pinMode(mosfetPin, OUTPUT);

  // Select the font to use with menu and all font functions
  //display.setFixedFont( ssd1306xled_font6x8 );
  display.setFixedFont( ssd1306xled_font8x16 );
  display.begin();
  display.clear();

  hasUpdate = true;
}

/* unsigned int testa = 5; */

void loop() {

/*
  display.clear();
  display.setTextCursor(0,0);
  //display.print("Timer:");

  display.write(uintToStr(testa));

  delay(5000);
*/

  updateKeys();

  doStateActions();

  // Update screen if needed
  if (hasUpdate) {
    updateScreen();
    hasUpdate = false;

    lastState = currentState;
  }

  delay(10);
}

// Handle keys
void keyPressed(Key keyPressed) {

  switch(currentState) {
    case STATE::SET_TIMER:
      updateTimerState(keyPressed);
      break;
    case STATE::WORKING:
      updateWorkingState(keyPressed);

      break;
  }

/*
  switch(keyPressed) {
    case Key::UP:
      Serial.println("UP");
      break;
    case Key::DOWN:
      Serial.println("DOWN");
      break;
    case Key::SELECT:
      Serial.println("SELECT");
      break;
    case Key::BACK:
      Serial.println("BACK");
      break;
    default:
      // No key pressed
      break;
  }
*/
}

void updateTimerState(Key key) {
  switch (key) {
    case Key::UP:
      if (minutes < 65535) { minutes += 5; hasUpdate = true; }
      break;
    case Key::DOWN:
      if (minutes > 0) {minutes += 5; hasUpdate = true; }
      break;
    case Key::SELECT:
      if (minutes > 0) {
        // TODO: Store minutes to memory
        currentState = STATE::WORKING;
        count = minutes;
        lastMillis = millis();
        hasUpdate = true;
        return;
      }
      break;
    case Key::BACK:
      // TODO: Go to deep sleep
      Serial.println(">>> DEEP SLEEP");
      // hasUpdate = true
      return;
      break;
  }

}

void updateWorkingState(Key key) {
  switch(key) {
    case Key::BACK:
      currentState = STATE::SET_TIMER;
      hasUpdate = true;
    break;
  }
}


// Main loop/actions update
void doStateActions() {

  if (currentState == STATE::WORKING) {

    // Countdown
    if (count > 0) {

      // ===== METHOD 1: Has "drift"
      /*
      currentMilis = millis();
      if (currentMilis - previousMilis >= 1000) {
        previousMilis = currentMilis
        count--;
      }
      */

      // ==== METHOD 2
      if (millis() - lastMillis >= 1000) {
        setMosfet(true);
        lastMillis += 1000;
        count--;
        hasUpdate = true;
      }

    } else {
      // Finish count
      setMosfet(false);
      currentState = STATE::SET_TIMER;
      hasUpdate = true;
    }
    
  }
}

// Screen updates
void updateScreen() {

  bool stateChanged = lastState != currentState;

  if (stateChanged)
    display.clear();

  switch (currentState) {
    case STATE::SET_TIMER:
      // LCD write "Timer"
      if (stateChanged)
        Serial.println("[SCREEN] SET_TIMER");
      Serial.print("[Info] Timer: ");
      Serial.println(minutes);

      if (stateChanged)
        display.printFixed(0, 8, "TIMER (MIN)");

      display.printFixed(0, 16, uintToStr(minutes), STYLE_BOLD);

      // Calculate Liters (use L/min from pump store specifications)
      display.printFixed(0, 24, uintToStr(minutes * 3), STYLE_BOLD);
      

      break;
    case STATE::WORKING:

      if (stateChanged)
        Serial.println("[SCREEN] WORKING");
      Serial.print("[Info] Count: ");
      Serial.println(count);

      // display.printFixed(0, 8, "WATERING:", STYLE_NORMAL);
      // display.printFixed(0, 16, uintToStr(count), STYLE_BOLD);

      // animationKey++;
      // if (animationKey >= 5) animationKey = 1; // Loop back
      // display.printFixed(0, 8 * animationKey, '|');

      if (stateChanged)
        display.printFixed(0, 0, "Watering", STYLE_NORMAL);

      int percent = map(count, minutes, 0, 0, 100);
      display.drawProgressBar(percent);
      break;
    // case STATE::FINISHED:
    //   if (stateChanged)
    //     Serial.println("[SCREEN] FINISHED");
    //   break;
  }

}

char* uintToStr(unsigned int value) {
  static char buffer[12]; // CAUTION: Reused between calls, not thread-safe
  snprintf(buffer, sizeof(buffer), "%u", value);
  return buffer;
}

void setMosfet(bool active) {
  digitalWrite(mosfetPin, active ? HIGH : LOW);
}
