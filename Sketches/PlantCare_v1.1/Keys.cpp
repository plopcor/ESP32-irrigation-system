#include <Arduino.h>
#include "Button2.h"
#include "Keys.h"

#pragma once

// Button2 initialization
static Button2 btnUp, btnDown, btnSelect, btnBack;

// Callback to send keys to
static KeyCallback userCallback = nullptr;

// Set handler for tap/click
static void tapHandler(Button2& btn)
{
  if (!userCallback) return;

  if (btn == btnUp)           userCallback(Key::UP);
  else if (btn == btnDown)    userCallback(Key::DOWN);
  else if (btn == btnSelect)  userCallback(Key::SELECT);
  else if (btn == btnBack)    userCallback(Key::BACK);

  // Debounce?
}

// Set pins and user callback
void setupKeys(
  uint8_t upPin,
  uint8_t downPin,
  uint8_t selectPin,
  uint8_t backPin
)
{
  // Map buttons to GPIO
  btnUp.begin(upPin);
  btnDown.begin(downPin);
  btnSelect.begin(selectPin);
  btnBack.begin(backPin);

  // Set internal handler
  btnUp.setTapHandler(tapHandler);
  btnDown.setTapHandler(tapHandler);
  btnSelect.setTapHandler(tapHandler);
  btnBack.setTapHandler(tapHandler);
}

void setupKeyCallback(KeyCallback callback)
{
  userCallback = callback;
}

// Update internal loops
void updateKeys()
{
  if (!userCallback) return;

  btnUp.loop();
  btnDown.loop();
  btnSelect.loop();
  btnBack.loop();
}

/*
Key readKey() {
  btnUp.loop();
  btnDown.loop();
  btnSelect.loop();
  btnBack.loop();

  if (btnUp.wasPressed() && btnUp.read())     return Key::UP;
  if (btnDown.wasPressed() && btnDown.read())   return Key::DOWN;
  if (btnSelect.wasPressed() && btnSelect.read()) return Key::SELECT;
  if (btnBack.wasPressed() && btnBack.read())   return Key::BACK;
  return Key::NONE;
}
*/