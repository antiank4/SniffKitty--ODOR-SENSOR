#include "storage.h"
#include "config.h"

#include <SD.h>
#include <SPI.h>

bool sdReady = false;

static SPIClass sdSPI(FSPI);

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
    file.println("CSV,time_ms,recording,pir,mq137_raw,voltage,ammonia_change_percent,status,temp_C,hum_percent,delta_temp,delta_hum");
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
