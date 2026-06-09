#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

extern const char* ssid;
extern const char* password;

const int MQ137_PIN = 1;
const int MQ135_PIN = 2;
const int AHT_SDA = 14;
const int AHT_SCL = 21;

const int SD_SCK_PIN = 42;
const int SD_MISO_PIN = 41;
const int SD_MOSI_PIN = 47;
const int SD_CS_PIN = 45;
const uint32_t SD_SPI_FREQ = 400000;
const char* const SD_LOG_PATH = "/gas_data.csv";
const bool SAVE_PRESENCE_PHOTO_ENABLED = true;
const unsigned long SD_RETRY_INTERVAL_MS = 5000;

const bool STATUS_LED_ENABLED = true;
const int STATUS_LED_PIN = 48;
const uint8_t STATUS_LED_BRIGHTNESS = 24;
const float LED_SLIGHT_CHANGE_PERCENT = 10.0;
const float LED_MEDIUM_CHANGE_PERCENT = 30.0;
const float LED_STRONG_CHANGE_PERCENT = 80.0;

const int SAMPLE_COUNT = 2;
const int DELAY_BETWEEN_SAMPLES = 2;
const int MQ135_SAMPLE_COUNT = 12;
const float MQ135_FILTER_ALPHA = 0.18;
const int MQ135_RAW_DEADBAND = 3;

const unsigned long PRESENCE_IGNORE_AFTER_TRIGGER = 5000;
const unsigned long PRESENCE_RECORD_AFTER_EMPTY_MS = 20000;

const unsigned long CAMERA_BASELINE_DELAY_MS = 10000;
const unsigned long CAMERA_CHECK_INTERVAL_MS = 1000;
const int CAMERA_COMPARE_SAMPLE_STEP = 8;
const int CAMERA_PIXEL_DIFF_THRESHOLD = 35;
const float CAMERA_PRESENT_CHANGE_PERCENT = 7.0;
const float CAMERA_EMPTY_CHANGE_PERCENT = 7.0;
const int CAMERA_PRESENT_SAMPLE_MIN = 2;
const int CAMERA_EMPTY_SAMPLE_MIN = 2;

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
