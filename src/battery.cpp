#include "battery.h"

#define ANALOG_BATTERY_PIN 36

/*
Battery measurement 
read analog value >> 4 (highest 8 bits) gives us a number 0-255.

Because of different ADC and resistors, we have 127/125 for 3.6V. 
The device stops to work on the value 127.

Table with my battery observations
        1(Red)        2(White)
USB disconnected, battery (discharged - 3.6V) connected
        127         125
USB connected, battery disconnected
        148+        142+
USB connected, battery (discharged) connected 
        139         134
USB disconnected, battery (charged - 4.15 / 4.13V) connected
        147         146 

*/

Battery::Battery() {
}

void Battery::update() {
    _value = analogRead(ANALOG_BATTERY_PIN) >> 4;   //8bits more than enough
}

uint16_t Battery::getValue() {
    return _value;
} 