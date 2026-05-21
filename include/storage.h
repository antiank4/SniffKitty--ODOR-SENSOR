#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

extern bool sdReady;

void initStorage();
void appendCSVLineToSD(const String& line);

#endif
