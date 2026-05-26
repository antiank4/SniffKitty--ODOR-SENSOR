#ifndef CAMERA_SERVER_H
#define CAMERA_SERVER_H

void initCamera();
void connectWiFi();
void startCameraServer();
void captureCameraBaselineAfterDelay();
bool updateCameraPresence();

extern bool cameraBaselineReady;
extern bool currentCameraPresent;
extern float currentCameraChangePercent;

#endif
