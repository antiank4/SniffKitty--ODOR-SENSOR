#include <Arduino.h>

#include "sensors.h"
#include "camera_server.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  initSensors();

  initCamera();

  connectWiFi();

  startCameraServer();

  calibrateBaseline();

  Serial.println("Waiting for PIR trigger...");
  Serial.println("CSV,time_ms,recording,pir,mq135_raw,voltage,odor_change_percent,status,temp_C,hum_percent,delta_temp,delta_hum");
}

void loop() {
  updateSensors();

  printSensorStatus();

  printCSVIfRecording();
}