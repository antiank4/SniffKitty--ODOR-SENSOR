#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

#define sensor_t adafruit_sensor_t
#include <Adafruit_AHTX0.h>
#undef sensor_t

extern int baseline;

extern bool recording;
extern bool postRecording;
extern unsigned long lastTriggerTime;
extern unsigned long recordingStartTime;
extern unsigned long postRecordingEndTime;

extern volatile bool pirInterruptTriggered;

extern Adafruit_AHTX0 aht;
extern bool ahtReady;

extern float baseTemp;
extern float baseHum;

extern float currentTempC;
extern float currentHum;
extern float currentDeltaTemp;
extern float currentDeltaHum;

extern int currentMQ135Raw;
extern float currentVoltage;
extern float currentChangePercent;
extern String currentStatus;
extern bool currentPirState;
extern int currentPirRawLevel;
extern int currentPirTriggerSamples;

void IRAM_ATTR handlePirInterrupt();

int readMQ135Average();
void calibrateBaseline();
void updateStatusFromMQ135();
void initSensors();
void updateSensors();
void printSensorStatus();
void printCSVIfRecording();

#endif
