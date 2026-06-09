#include "storage.h"
#include "config.h"

#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

bool sdReady = false;

static SPIClass sdSPI(FSPI);
static bool sdSpiStarted = false;
static unsigned long lastSdRetryTime = 0;
static bool sdRetryMessagePrinted = false;
static bool clockSynced = false;
static unsigned long lastClockSyncAttemptTime = 0;

static void markSdOffline(const char* reason) {
  if (sdReady) {
    Serial.println(reason);
  }

  sdReady = false;
  SD.end();
}

static bool beginStorage(bool verbose) {
  if (verbose) {
    Serial.println("SD init start...");
    Serial.print("SD pins SCK/MISO/MOSI/CS: ");
    Serial.print(SD_SCK_PIN);
    Serial.print("/");
    Serial.print(SD_MISO_PIN);
    Serial.print("/");
    Serial.print(SD_MOSI_PIN);
    Serial.print("/");
    Serial.println(SD_CS_PIN);
  }

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(50);

  if (!sdSpiStarted) {
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    sdSpiStarted = true;
  }

  delay(50);

  if (!SD.begin(SD_CS_PIN, sdSPI, SD_SPI_FREQ)) {
    sdReady = false;
    if (verbose) {
      Serial.println("SD init failed. Offline CSV logging disabled.");
    }
    return false;
  }

  File file = SD.open(SD_LOG_PATH, FILE_APPEND);
  if (!file) {
    markSdOffline("SD log file open failed. Offline CSV logging disabled.");
    return false;
  }

  if (file.size() == 0) {
    file.println("CSV,timestamp,time_ms,recording,present,mq137_raw,mq137_voltage,ammonia_change_percent,ammonia_status,mq135_raw,mq135_voltage,air_change_percent,air_status,temp_C,hum_percent,delta_temp,delta_hum");
  }

  file.close();
  sdReady = true;
  sdRetryMessagePrinted = false;

  if (verbose) {
    Serial.print("SD logging ready: ");
    Serial.println(SD_LOG_PATH);
  } else {
    Serial.println("SD card reconnected. Logging resumed.");
  }

  return true;
}

static bool getLocalTimeInfo(struct tm* timeinfo) {
  if (!getLocalTime(timeinfo, 100)) {
    return false;
  }

  return (timeinfo->tm_year + 1900) >= 2024;
}

void syncClockFromNTP() {
  lastClockSyncAttemptTime = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Clock sync skipped: WiFi not connected.");
    return;
  }

  Serial.println("Syncing clock from NTP...");
  configTzTime("PST8PDT,M3.2.0,M11.1.0", "time.google.com", "pool.ntp.org", "time.cloudflare.com");

  struct tm timeinfo;
  unsigned long startTime = millis();

  while (millis() - startTime < CLOCK_SYNC_TIMEOUT_MS) {
    if (getLocalTimeInfo(&timeinfo)) {
      char buffer[24];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.print("Clock synced: ");
      Serial.println(buffer);
      clockSynced = true;
      return;
    }

    Serial.print(".");
    delay(250);
  }

  Serial.println();
  Serial.println("Clock sync not ready. Falling back to millis timestamps.");
}

String getTimestampString() {
  struct tm timeinfo;
  if (getLocalTimeInfo(&timeinfo)) {
    char buffer[24];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
  }

  return "ms_" + String(millis());
}

String getTimestampFilename() {
  struct tm timeinfo;
  if (getLocalTimeInfo(&timeinfo)) {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &timeinfo);
    return String(buffer);
  }

  return "ms_" + String(millis());
}

void initStorage() {
  beginStorage(true);
  lastSdRetryTime = millis();
}

void updateStorage() {
  if (!clockSynced && WiFi.status() == WL_CONNECTED && millis() - lastClockSyncAttemptTime >= CLOCK_RETRY_INTERVAL_MS) {
    syncClockFromNTP();
  }

  if (sdReady) {
    return;
  }

  if (!sdRetryMessagePrinted) {
    Serial.println("SD offline. Will retry automatically.");
    sdRetryMessagePrinted = true;
  }

  if (millis() - lastSdRetryTime < SD_RETRY_INTERVAL_MS) {
    return;
  }

  lastSdRetryTime = millis();
  beginStorage(false);
}

void appendCSVLineToSD(const String& line) {
  if (!sdReady) {
    return;
  }

  File file = SD.open(SD_LOG_PATH, FILE_APPEND);
  if (!file) {
    markSdOffline("SD write failed. Offline CSV logging disabled.");
    return;
  }

  file.println(line);
  file.close();
}

String saveJpegPhotoToSD(const uint8_t* data, size_t len) {
  if (!sdReady || data == NULL || len == 0) {
    return "";
  }

  const char* photoDir = "/photos";
  if (!SD.exists(photoDir)) {
    SD.mkdir(photoDir);
  }

  String baseName = getTimestampFilename();
  String path = String(photoDir) + "/" + baseName + ".jpg";
  int suffix = 1;

  while (SD.exists(path) && suffix < 100) {
    path = String(photoDir) + "/" + baseName + "_" + String(suffix) + ".jpg";
    suffix++;
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    markSdOffline("Photo save failed: file open error");
    return "";
  }

  size_t written = file.write(data, len);
  file.close();

  if (written != len) {
    markSdOffline("Photo save failed: incomplete write");
    return "";
  }

  Serial.print("Photo saved: ");
  Serial.println(path);
  return path;
}
