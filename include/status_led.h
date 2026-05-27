#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>

void initStatusLed();
void setStatusLedBlue();
void clearStatusLed();
void blinkStatusLedBlue(uint8_t pulses, unsigned long onMs, unsigned long offMs);
void updateStatusLedFromAmmonia(float changePercent);

#endif
