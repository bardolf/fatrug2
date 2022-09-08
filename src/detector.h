#ifndef detector_h
#define detector_h

#include <Arduino.h>

#define TRIGGER_PIN 26
#define ECHO_PIN 25
#define RANGE_THRESHOLD_CM 80
#define SOUND_SPEED_HALF 0.017

typedef enum {
  NONE,
  ARRIVED,
  LEFT
} DetectedObjectState;

class Detector {
   public:
    Detector();
    void init();
    DetectedObjectState read();
    void startMeasurement();
    void stopMeasurement();    

   private:
    bool _prevObjectDetected;
    bool _measurementEnabled = false;
    float measureDistance(); 
    float _prevDistance;   
};

#endif
