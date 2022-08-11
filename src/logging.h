#ifndef logging_h
#define logging_h

#include <Arduino.h>
#include <ArduinoLog.h>

extern void printPrefix(Print* _logOutput, int logLevel);
extern void printTimestamp(Print* _logOutput);
extern void printLogLevel(Print* _logOutput, int logLevel);

#endif