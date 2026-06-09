#include "sensors.h"
#include "camera_server.h"
#include "config.h"
#include "storage.h"
#include "status_led.h"
#include <math.h>
#include <Wire.h>

int baseline = 0;
int mq135Baseline = 0;

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
int currentMQ135Raw = 0;
float currentMQ135Voltage = 0;
float currentMQ135ChangePercent = 0;
String currentMQ135Status = "Waiting";
bool currentPresenceState = false;
static bool pendingPresencePhoto = false;

static int readGasAverage(int pin, int sampleCount) {
  long sum = 0;
  int minVal = 4095;
  int maxVal = 0;

  for (int i = 0; i < sampleCount; i++) {
    int val = analogRead(pin);
    sum += val;
    minVal = min(minVal, val);
    maxVal = max(maxVal, val);
    delay(DELAY_BETWEEN_SAMPLES);
  }

  if (sampleCount > 2) {
    sum -= minVal;
    sum -= maxVal;
    return sum / (sampleCount - 2);
  }

  return sum / sampleCount;
}

int readMQ137Average() {
  return readGasAverage(MQ137_PIN, SAMPLE_COUNT);
}

int readMQ135Average() {
  return readGasAverage(MQ135_PIN, MQ135_SAMPLE_COUNT);
}

void calibrateBaseline() {
  Serial.println("Calibrating MQ137 + MQ135... keep air clean");

  unsigned long calibrationTime = 30000; // 10秒，可以改成 15000 或 30000
  unsigned long startTime = millis();

  long mq137Sum = 0;
  long mq135Sum = 0;
  int count = 0;

  while (millis() - startTime < calibrationTime) {
    mq137Sum += readMQ137Average();
    mq135Sum += readMQ135Average();
    count++;

    Serial.print(".");
    delay(50);
  }

  if (count > 0) {
    baseline = mq137Sum / count;
    mq135Baseline = mq135Sum / count;
  }

  Serial.println();
  Serial.print("MQ137 Baseline = ");
  Serial.println(baseline);
  Serial.print("MQ135 Baseline = ");
  Serial.println(mq135Baseline);
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

void updateStatusFromMQ135() {
  if (currentMQ135ChangePercent < 10) {
    currentMQ135Status = "Normal";
  } else if (currentMQ135ChangePercent < 30) {
    currentMQ135Status = "Slight Change";
  } else if (currentMQ135ChangePercent < 80) {
    currentMQ135Status = "Medium Air Change";
  } else {
    currentMQ135Status = "Strong Air Change!";
  }
}

void initSensors() {
  analogReadResolution(12);
  analogSetPinAttenuation(MQ137_PIN, ADC_11db);
  analogSetPinAttenuation(MQ135_PIN, ADC_11db);

  Wire.begin(AHT_SDA, AHT_SCL);

  Serial.println("MQ137 + MQ135 + camera presence + AHT10 + Web Dashboard start...");

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
    if (!previousPresenceState) {
      pendingPresencePhoto = true;
    }

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
  bool mq135PostCloseBoost = false;
  if (postRecording && postRecordingEndTime > millis()) {
    unsigned long postCloseElapsedMs = PRESENCE_RECORD_AFTER_EMPTY_MS - (postRecordingEndTime - millis());
    mq135PostCloseBoost = postCloseElapsedMs < MQ135_POST_CLOSE_BOOST_MS;
  }
  bool mq135ActiveMode = currentPresenceState || (recording && !postRecording) || mq135PostCloseBoost;
  float mq135FilterAlpha = mq135ActiveMode ? MQ135_ACTIVE_FILTER_ALPHA : MQ135_IDLE_FILTER_ALPHA;
  int mq135RawDeadband = mq135ActiveMode ? MQ135_ACTIVE_RAW_DEADBAND : MQ135_IDLE_RAW_DEADBAND;
  float mq135ChangeMultiplier = mq135ActiveMode ? MQ135_ACTIVE_CHANGE_MULTIPLIER : MQ135_IDLE_CHANGE_MULTIPLIER;

  int mq135RawSample = readMQ135Average();
  if (currentMQ135Raw == 0) {
    currentMQ135Raw = mq135RawSample;
  } else if (abs(mq135RawSample - currentMQ135Raw) > mq135RawDeadband) {
    currentMQ135Raw = round((mq135FilterAlpha * mq135RawSample) + ((1.0 - mq135FilterAlpha) * currentMQ135Raw));
  }
  currentMQ135Voltage = currentMQ135Raw * (3.3 / 4095.0);

  if (baseline > 0) {
    currentChangePercent = ((float)(currentMQ137Raw - baseline) / baseline) * 100 * MQ137_CHANGE_MULTIPLIER;
  } else {
    currentChangePercent = 0;
  }

  if (mq135Baseline > 0) {
    float mq135DeltaPercent = ((float)(currentMQ135Raw - mq135Baseline) / mq135Baseline) * 100;
    currentMQ135ChangePercent = fabs(mq135DeltaPercent) * mq135ChangeMultiplier;
    if (!mq135ActiveMode && currentMQ135ChangePercent < MQ135_IDLE_NOISE_FLOOR_PERCENT) {
      currentMQ135ChangePercent = 0;
    }
  } else {
    currentMQ135ChangePercent = 0;
  }

  updateStatusFromMQ137();
  updateStatusFromMQ135();
  updateStatusLedFromAmmonia(max(currentChangePercent, currentMQ135ChangePercent));

  if (pendingPresencePhoto && SAVE_PRESENCE_PHOTO_ENABLED) {
    pendingPresencePhoto = false;
    String photoPath = capturePresencePhotoToSD();
    if (photoPath.length() > 0) {
      Serial.print("Presence photo linked to visit: ");
      Serial.println(photoPath);
    }
  } else if (pendingPresencePhoto) {
    pendingPresencePhoto = false;
  }
}

void printSensorStatus() {
  Serial.print("Temp: ");
  Serial.print(currentTempC, 2);
  Serial.print(" C | Hum: ");
  Serial.print(currentHum, 2);
  Serial.print(" % | MQ137: ");
  Serial.print(currentMQ137Raw);
  Serial.print(" (");
  Serial.print(currentChangePercent, 1);
  Serial.print("%)");
  Serial.print(" | MQ135: ");
  Serial.print(currentMQ135Raw);
  Serial.print(" (");
  Serial.print(currentMQ135ChangePercent, 1);
  Serial.print("%)");
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
  Serial.print(" | MQ135 Status: ");
  Serial.print(currentMQ135Status);

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
  csvLine += getTimestampString();
  csvLine += ",";
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
  csvLine += String(currentMQ135Raw);
  csvLine += ",";
  csvLine += String(currentMQ135Voltage, 3);
  csvLine += ",";
  csvLine += String(currentMQ135ChangePercent, 1);
  csvLine += ",";
  csvLine += currentMQ135Status;
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
