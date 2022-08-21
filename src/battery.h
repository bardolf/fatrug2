#ifndef battery_h
#define battery_h

#include <Arduino.h>

class Battery {
   public:
    Battery();

    void update();

    uint16_t getValue();
    u_int8_t getPercentage();

   private:
    uint16_t _value;
};

#endif