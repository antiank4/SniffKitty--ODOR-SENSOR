#include "sensors.h"
#include "camera_server.h"
#include "config.h"
#include "storage.h"
#include "status_led.h"
#include <Wire.h>

int baseline = 0;

bool recording = false;
unsigned long lastTriggerTime = 0;
unsigned long recordingStartTime = 0;
bool postRecording = false;
unsigned long postRecordingEndTime = 0;

Adafruit_AHTX0 aht;
bool ahtReady = false;

float baseTemp = 0;
float baseHum = 0;

float currentTempC = 0;
float currentHum = 0;
float currentDeltaTemp = 0;
float currentDeltaHum = 0;

int currentMQ137Raw = 0;
float currentVoltage = 0;
float currentChangePercent = 0;
String currentStatus = "Waiting";
bool currentPresenceState = false;

int readMQ137Average() {
  long sum = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(MQ137_PIN);
    delay(DELAY_BETWEEN_SAMPLES);
  }

  return sum / SAMPLE_COUNT;
}

void calibrateBaseline() {
  Serial.println("Calibrating... keep air clean");

  unsigned long calibrationTime = 30000; // 10秒，可以改成 15000 或 30000
  unsigned long startTime = millis();

  long sum = 0;
  int count = 0;

  while (millis() - startTime < calibrationTime) {
    int val = readMQ137Average();
    sum += val;
    count++;

    Serial.print(".");
    delay(50);
  }

  if (count > 0) {
    baseline = sum / count;
  }

  Serial.println();
  Serial.print("Baseline = ");
  Serial.println(baseline);
}

void updateStatusFromMQ137() {
  if (currentChangePercent < 10) {
    currentStatus = "Normal";
  } else if (currentChangePercent < 30) {
    currentStatus = "Slight Change";
  } else if (currentChangePercent < 80) {
    currentStatus = "Medium Ammonia";
  } else {
    currentStatus = "Strong Ammonia!";
  }
}

void initSensors() {
  analogReadResolution(12);

  Wire.begin(AHT_SDA, AHT_SCL);

  Serial.println("MQ137 + camera presence + AHT10 + Web Dashboard start...");

  if (!aht.begin()) {
    Serial.println("AHT10 not found!");
    ahtReady = false;
  } else {
    Serial.println("AHT10 found!");
    ahtReady = true;
  }
}

void updateSensors() {
  bool previousPresenceState = currentPresenceState;
  currentPresenceState = updateCameraPresence();

  if (currentPresenceState) {
    if (postRecording) {
      postRecording = false;
      Serial.println("Camera present: cancel post-empty recording");
    }

    if (!recording) {
      recording = true;
      lastTriggerTime = millis();
      recordingStartTime = millis();

      Serial.println("Camera present: START recording");
      Serial.print("Ignoring first ");
      Serial.print(PRESENCE_IGNORE_AFTER_TRIGGER / 1000);
      Serial.println(" seconds of data after present trigger");

      if (ahtReady) {
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);

        baseTemp = temp.temperature;
        baseHum = humidity.relative_humidity;
      }
    }
  } else if (previousPresenceState && recording && !postRecording) {
    lastTriggerTime = millis();

    if (millis() - recordingStartTime >= PRESENCE_IGNORE_AFTER_TRIGGER) {
      postRecording = true;
      postRecordingEndTime = millis() + PRESENCE_RECORD_AFTER_EMPTY_MS;
      Serial.print("Camera empty: post recording for ");
      Serial.print(PRESENCE_RECORD_AFTER_EMPTY_MS / 1000);
      Serial.println(" seconds");
    } else {
      recording = false;
      Serial.println("Camera empty: STOP recording short trigger ignored");
    }
  }

  if (postRecording && millis() >= postRecordingEndTime) {
    postRecording = false;
    recording = false;
    Serial.println("Post recording complete: STOP recording");
  }

  if (ahtReady) {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    currentTempC = temp.temperature;
    currentHum = humidity.relative_humidity;

    if (recording) {
      currentDeltaTemp = currentTempC - baseTemp;
      currentDeltaHum = currentHum - baseHum;
    } else {
      currentDeltaTemp = 0;
      currentDeltaHum = 0;
    }
  }

  currentMQ137Raw = readMQ137Average();
  currentVoltage = currentMQ137Raw * (3.3 / 4095.0);

  if (baseline > 0) {
    currentChangePercent = ((float)(currentMQ137Raw - baseline) / baseline) * 100;
  } else {
    currentChangePercent = 0;
  }

  updateStatusFromMQ137();
  updateStatusLedFromAmmonia(currentChangePercent);
}

void printSensorStatus() {
  Serial.print("Temp: ");
  Serial.print(currentTempC, 2);
  Serial.print(" C | Hum: ");
  Serial.print(currentHum, 2);
  Serial.print(" % | MQ137: ");
  Serial.print(currentMQ137Raw);
  Serial.print(" | Camera: ");
  Serial.print(currentPresenceState ? "present" : "empty");
  Serial.print(" | Camera change: ");
  Serial.print(currentCameraChangePercent, 1);
  Serial.print("%");
  Serial.print(" | Recording: ");
  if (postRecording) {
    unsigned long remainingMs = 0;
    if (postRecordingEndTime > millis()) {
      remainingMs = postRecordingEndTime - millis();
    }

    Serial.print("post-close ");
    Serial.print((remainingMs + 999) / 1000);
    Serial.print("s left");
  } else {
    Serial.print(recording ? "active" : "off");
  }
  Serial.print(" | Status: ");
  Serial.print(currentStatus);

  if (recording) {
    Serial.print(" | Delta Temp: ");
    Serial.print(currentDeltaTemp, 2);
    Serial.print(" C | Delta Hum: ");
    Serial.print(currentDeltaHum, 2);
    Serial.print(" %");
  }

  Serial.println();
}

void printCSVIfRecording() {
  if (!recording) {
    delay(500);
    return;
  }

  if (millis() - recordingStartTime < PRESENCE_IGNORE_AFTER_TRIGGER) {
    Serial.println("Ignoring unstable data after present trigger...");
    delay(1000);
    return;
  }

  String csvLine = "CSV,";
  csvLine += String(millis());
  csvLine += ",";
  csvLine += String(recording ? 1 : 0);
  csvLine += ",";
  csvLine += String(currentPresenceState ? 1 : 0);
  csvLine += ",";
  csvLine += String(currentMQ137Raw);
  csvLine += ",";
  csvLine += String(currentVoltage, 3);
  csvLine += ",";
  csvLine += String(currentChangePercent, 1);
  csvLine += ",";
  csvLine += currentStatus;
  csvLine += ",";
  csvLine += String(currentTempC, 2);
  csvLine += ",";
  csvLine += String(currentHum, 2);
  csvLine += ",";
  csvLine += String(currentDeltaTemp, 2);
  csvLine += ",";
  csvLine += String(currentDeltaHum, 2);

  Serial.println(csvLine);
  appendCSVLineToSD(csvLine);

  delay(500);
}
