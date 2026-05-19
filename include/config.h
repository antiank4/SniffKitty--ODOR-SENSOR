#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

extern const char* ssid;
extern const char* password;

const int MQ135_PIN = 1;
const int PIR_PIN = 2;
const int PIR_ACTIVE_LEVEL = HIGH;

const int AHT_SDA = 14;
const int AHT_SCL = 21;

const int SAMPLE_COUNT = 2;
const int DELAY_BETWEEN_SAMPLES = 2;

const unsigned long PIR_COOLDOWN = 0;
const unsigned long PIR_IGNORE_AFTER_TRIGGER = 5000;
const int PIR_TRIGGER_SAMPLE_MIN = 3;
const int PIR_RELEASE_SAMPLE_MIN = 2;
const unsigned long PIR_EVENT_LOCKOUT_MS = 3000;

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

#endif
