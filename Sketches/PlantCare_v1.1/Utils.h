#pragma once

#include <sys/_stdint.h>

char* int8ToChars(int8_t value);
char* uint8ToChars(uint8_t value, bool padRight = false);
char* uint16ToChars(uint16_t value, bool padRight = false);
char* byteToChar(unsigned int value);

void setupDeepSleep(int pin);
void doDeepSleep();