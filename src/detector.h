#ifndef detector_h
#define detector_h

#include <Adafruit_VL53L0X.h>
#include <Arduino.h>
#include <Wire.h>

class Detector {
   public:
    Detector();
    bool init();
    void update();
    void startMeasurement();
    void stopMeasurement();
    bool isObjectLeft();
    bool isObjectArrived();

   private:
    Adafruit_VL53L0X* _lox;
    bool _measurementEnabled = false;
    bool _firstMeasurement = true;
    bool _currObjectDetected;
    bool _prevObjectDetected;
    bool _fixPrevObjectDetected;
};

#endif