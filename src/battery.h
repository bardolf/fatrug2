#ifndef battery_h
#define battery_h

#include <Arduino.h>

class Battery {
   public:
    Battery();

    void update();
    uint8_t getValue();
    int getPercentage();

   private:
    uint8_t _value;
    int computePercentage(uint8_t value);
};

#endif