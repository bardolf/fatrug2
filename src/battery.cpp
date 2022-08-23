#include "battery.h"

#define ANALOG_BATTERY_PIN 36

/*
Battery measurement 
read analog value >> 4 (highest 8 bits) gives us a number 0-255.

Because of different ADC and resistors, we have 127/125 for 3.6V. 
The device stops to work on the value 127.
*/

Battery::Battery() {
}

void Battery::update() {
    _value = analogRead(ANALOG_BATTERY_PIN) >> 4;   //8bits more than enough
}

uint16_t Battery::getValue() {
    return _value;
} 