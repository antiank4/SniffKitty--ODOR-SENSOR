#include <Arduino.h>

#include "sensors.h"
#include "camera_server.h"
#include "storage.h"
#include "status_led.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  initSensors();

  initStatusLed();

  initStorage();

  initCamera();

  connectWiFi();

  startCameraServer();

  captureCameraBaselineAfterDelay();

  calibrateBaseline();

  Serial.println("Waiting for camera presence...");
  Serial.println("CSV,timestamp,time_ms,recording,present,mq137_raw,mq137_voltage,ammonia_change_percent,ammonia_status,mq135_raw,mq135_voltage,air_change_percent,air_status,temp_C,hum_percent,delta_temp,delta_hum");
}

void loop() {
  updateStorage();

  updateSensors();

  printSensorStatus();

  printCSVIfRecording();
}
