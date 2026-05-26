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

extern Adafruit_AHTX0 aht;
extern bool ahtReady;

extern float baseTemp;
extern float baseHum;

extern float currentTempC;
extern float currentHum;
extern float currentDeltaTemp;
extern float currentDeltaHum;

extern int currentMQ137Raw;
extern float currentVoltage;
extern float currentChangePercent;
extern String currentStatus;
extern bool currentPresenceState;

int readMQ137Average();
void calibrateBaseline();
void updateStatusFromMQ137();
void initSensors();
void updateSensors();
void printSensorStatus();
void printCSVIfRecording();

#endif
