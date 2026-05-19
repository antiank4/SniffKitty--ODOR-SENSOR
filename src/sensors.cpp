#include "sensors.h"
#include "config.h"
#include <Wire.h>

int baseline = 0;

bool recording = false;
unsigned long lastTriggerTime = 0;
unsigned long recordingStartTime = 0;

volatile bool pirInterruptTriggered = false;

Adafruit_AHTX0 aht;
bool ahtReady = false;

float baseTemp = 0;
float baseHum = 0;

float currentTempC = 0;
float currentHum = 0;
float currentDeltaTemp = 0;
float currentDeltaHum = 0;

int currentMQ135Raw = 0;
float currentVoltage = 0;
float currentChangePercent = 0;
String currentStatus = "Waiting";
bool currentPirState = false;
int currentPirRawLevel = HIGH;
int currentPirTriggerSamples = 0;

void IRAM_ATTR handlePirInterrupt() {
  pirInterruptTriggered = true;
}

int readMQ135Average() {
  long sum = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(MQ135_PIN);
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
    int val = readMQ135Average();
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

void updateStatusFromMQ135() {
  if (currentChangePercent < 10) {
    currentStatus = "Normal";
  } else if (currentChangePercent < 30) {
    currentStatus = "Slight Change";
  } else if (currentChangePercent < 80) {
    currentStatus = "Medium Odor";
  } else {
    currentStatus = "Strong Odor!";
  }
}

void initSensors() {
  analogReadResolution(12);

  pinMode(PIR_PIN, INPUT);

  Wire.begin(AHT_SDA, AHT_SCL);

  Serial.println("MQ135 + PIR interrupt + AHT10 + Web Dashboard start...");

  if (!aht.begin()) {
    Serial.println("AHT10 not found!");
    ahtReady = false;
  } else {
    Serial.println("AHT10 found!");
    ahtReady = true;
  }
}

void updateSensors() {
  static int activeSampleCount = 0;
  static int clearSampleCount = PIR_RELEASE_SAMPLE_MIN;
  static bool pirArmed = true;
  static unsigned long lastPirEventTime = 0;

  currentPirRawLevel = digitalRead(PIR_PIN);
  bool rawPirState = (currentPirRawLevel == PIR_ACTIVE_LEVEL);
  bool pirEvent = false;

  if (rawPirState) {
    activeSampleCount++;
    clearSampleCount = 0;
  } else {
    activeSampleCount = 0;
    clearSampleCount++;

    if (clearSampleCount >= PIR_RELEASE_SAMPLE_MIN) {
      pirArmed = true;
    }
  }

  if (
    pirArmed &&
    activeSampleCount >= PIR_TRIGGER_SAMPLE_MIN &&
    millis() - lastPirEventTime >= PIR_EVENT_LOCKOUT_MS
  ) {
    pirEvent = true;
    pirArmed = false;
    lastPirEventTime = millis();
    activeSampleCount = 0;
  }

  if (pirEvent) {
    currentPirState = !currentPirState;
  }

  currentPirTriggerSamples = activeSampleCount;

  noInterrupts();
  pirInterruptTriggered = false;
  interrupts();

  if (currentPirState) {
    if (!recording && millis() - lastTriggerTime > PIR_COOLDOWN) {
      recording = true;
      lastTriggerTime = millis();
      recordingStartTime = millis();

      Serial.println("PIR detected: START recording");
      Serial.print("Ignoring first ");
      Serial.print(PIR_IGNORE_AFTER_TRIGGER / 1000);
      Serial.println(" seconds of data after PIR trigger");

      if (ahtReady) {
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);

        baseTemp = temp.temperature;
        baseHum = humidity.relative_humidity;
      }
    }
  } else if (recording) {
    recording = false;
    lastTriggerTime = millis();
    Serial.println("PIR clear: STOP recording");
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

  currentMQ135Raw = readMQ135Average();
  currentVoltage = currentMQ135Raw * (3.3 / 4095.0);

  if (baseline > 0) {
    currentChangePercent = ((float)(currentMQ135Raw - baseline) / baseline) * 100;
  } else {
    currentChangePercent = 0;
  }

  updateStatusFromMQ135();
}

void printSensorStatus() {
  Serial.print("Temp: ");
  Serial.print(currentTempC, 2);
  Serial.print(" C | Hum: ");
  Serial.print(currentHum, 2);
  Serial.print(" % | MQ135: ");
  Serial.print(currentMQ135Raw);
  Serial.print(" | PIR: ");
  Serial.print(currentPirState ? 1 : 0);
  Serial.print(" | PIR raw: ");
  Serial.print(currentPirRawLevel);
  Serial.print(" | PIR trigger samples: ");
  Serial.print(currentPirTriggerSamples);
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
  if (!recording || !currentPirState) {
    delay(500);
    return;
  }

  if (millis() - recordingStartTime < PIR_IGNORE_AFTER_TRIGGER) {
    Serial.println("Ignoring unstable data after PIR trigger...");
    delay(1000);
    return;
  }

  Serial.print("CSV,");
  Serial.print(millis());
  Serial.print(",");
  Serial.print(recording ? 1 : 0);
  Serial.print(",");
  Serial.print(currentPirState ? 1 : 0);
  Serial.print(",");
  Serial.print(currentMQ135Raw);
  Serial.print(",");
  Serial.print(currentVoltage, 3);
  Serial.print(",");
  Serial.print(currentChangePercent, 1);
  Serial.print(",");
  Serial.print(currentStatus);
  Serial.print(",");
  Serial.print(currentTempC, 2);
  Serial.print(",");
  Serial.print(currentHum, 2);
  Serial.print(",");
  Serial.print(currentDeltaTemp, 2);
  Serial.print(",");
  Serial.println(currentDeltaHum, 2);

  delay(500);
}
