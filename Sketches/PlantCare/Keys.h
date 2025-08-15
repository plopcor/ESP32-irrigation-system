#include <stdint.h>

//#ifndef BUTTONS_HEADER
//#define BUTTONS_HEADER


#pragma once

enum class Key: uint8_t
{
  UP,
  DOWN,
  SELECT,
  BACK,
  // NONE
};

// Declare callback function type (called when key is pressed)
typedef void (*KeyCallback)(Key);

// Call in setup()
void setupKeys(
  uint8_t upPin,
  uint8_t downPin,
  uint8_t selectPin,
  uint8_t backPin
);

// Set user callback
void setupKeyCallback(KeyCallback callback);

// Call in loop()
void updateKeys();

// #endif