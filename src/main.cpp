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

  calibrateBaseline();

  Serial.println("Waiting for PIR trigger...");
  Serial.println("CSV,time_ms,recording,pir,mq137_raw,voltage,ammonia_change_percent,status,temp_C,hum_percent,delta_temp,delta_hum");
}

void loop() {
  updateSensors();

  printSensorStatus();

  printCSVIfRecording();
}
