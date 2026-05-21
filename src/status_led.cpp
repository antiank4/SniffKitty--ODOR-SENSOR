#include "status_led.h"
#include "config.h"

#include <Arduino.h>

static void setStatusLed(uint8_t red, uint8_t green, uint8_t blue) {
  if (!STATUS_LED_ENABLED) {
    return;
  }

  neopixelWrite(STATUS_LED_PIN, red, green, blue);
}

void initStatusLed() {
  if (!STATUS_LED_ENABLED) {
    return;
  }

  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatusLed(0, 0, STATUS_LED_BRIGHTNESS);
}

void updateStatusLedFromOdor(float changePercent) {
  if (!STATUS_LED_ENABLED) {
    return;
  }

  if (changePercent < LED_SLIGHT_CHANGE_PERCENT) {
    setStatusLed(0, STATUS_LED_BRIGHTNESS, 0);
  } else if (changePercent < LED_MEDIUM_CHANGE_PERCENT) {
    setStatusLed(STATUS_LED_BRIGHTNESS, STATUS_LED_BRIGHTNESS, 0);
  } else if (changePercent < LED_STRONG_CHANGE_PERCENT) {
    setStatusLed(STATUS_LED_BRIGHTNESS, STATUS_LED_BRIGHTNESS / 3, 0);
  } else {
    setStatusLed(STATUS_LED_BRIGHTNESS, 0, 0);
  }
}
