#pragma once

#include "Utils.h"
#include <stdio.h>
#include <sys/_stdint.h>
#include "Arduino.h"

// CAUTION: Conversion buffers are reused between calls, not thread-safe

char* int8ToChars(int8_t value) {
  static char buffer[6]; // Enough for "-128" (4 chars) + null terminator
  snprintf(buffer, sizeof(buffer), "%d", value);
  return buffer;
}

char* uint8ToChars(uint8_t value, bool padRight) {  
  static char buffer[4]; // Enough for "255" + null terminator
  snprintf(buffer, sizeof(buffer), padRight ? "%-3u" : "%u", value); // Use "-3" to right-pad with spaces 
  return buffer;
}

char* uint16ToChars(uint16_t value, bool padRight) {
  static char buffer[12];
  snprintf(buffer, sizeof(buffer), padRight ? "%-11u" : "%u", value);
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

    esp_err_t res = esp_deep_sleep_enable_gpio_wakeup(1 << pin, ESP_GPIO_WAKEUP_GPIO_LOW);

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

