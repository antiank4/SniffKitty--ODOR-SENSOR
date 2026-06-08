#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

extern bool sdReady;

void initStorage();
void syncClockFromNTP();
String getTimestampString();
String getTimestampFilename();
void appendCSVLineToSD(const String& line);
String saveJpegPhotoToSD(const uint8_t* data, size_t len);

#endif
