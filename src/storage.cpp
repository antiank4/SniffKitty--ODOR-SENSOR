#include "storage.h"
#include "config.h"

#include <SD.h>
#include <SPI.h>
#include <time.h>

bool sdReady = false;

static SPIClass sdSPI(FSPI);

static bool getLocalTimeInfo(struct tm* timeinfo) {
  return getLocalTime(timeinfo, 10);
}

void syncClockFromNTP() {
  Serial.println("Syncing clock from NTP...");
  configTzTime("PST8PDT,M3.2.0,M11.1.0", "pool.ntp.org", "time.nist.gov", "time.google.com");

  struct tm timeinfo;
  if (getLocalTimeInfo(&timeinfo)) {
    char buffer[24];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.print("Clock synced: ");
    Serial.println(buffer);
  } else {
    Serial.println("Clock sync not ready. Falling back to millis timestamps.");
  }
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
  Serial.println("SD init start...");
  Serial.print("SD pins SCK/MISO/MOSI/CS: ");
  Serial.print(SD_SCK_PIN);
  Serial.print("/");
  Serial.print(SD_MISO_PIN);
  Serial.print("/");
  Serial.print(SD_MOSI_PIN);
  Serial.print("/");
  Serial.println(SD_CS_PIN);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  delay(50);

  sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  delay(50);

  if (!SD.begin(SD_CS_PIN, sdSPI, SD_SPI_FREQ)) {
    sdReady = false;
    Serial.println("SD init failed. Offline CSV logging disabled.");
    return;
  }

  File file = SD.open(SD_LOG_PATH, FILE_APPEND);
  if (!file) {
    sdReady = false;
    Serial.println("SD log file open failed. Offline CSV logging disabled.");
    return;
  }

  if (file.size() == 0) {
    file.println("CSV,timestamp,time_ms,recording,present,mq137_raw,mq137_voltage,ammonia_change_percent,ammonia_status,mq135_raw,mq135_voltage,air_change_percent,air_status,temp_C,hum_percent,delta_temp,delta_hum");
  }

  file.close();
  sdReady = true;

  Serial.print("SD logging ready: ");
  Serial.println(SD_LOG_PATH);
}

void appendCSVLineToSD(const String& line) {
  if (!sdReady) {
    return;
  }

  File file = SD.open(SD_LOG_PATH, FILE_APPEND);
  if (!file) {
    sdReady = false;
    Serial.println("SD write failed. Offline CSV logging disabled.");
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
    Serial.println("Photo save failed: file open error");
    return "";
  }

  size_t written = file.write(data, len);
  file.close();

  if (written != len) {
    Serial.println("Photo save failed: incomplete write");
    return "";
  }

  Serial.print("Photo saved: ");
  Serial.println(path);
  return path;
}
