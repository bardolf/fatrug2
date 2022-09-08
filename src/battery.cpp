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
        147         145
*/
Battery::Battery() {
}

void Battery::update() {
    _value = analogRead(ANALOG_BATTERY_PIN) >> 4;  // 8bits more than enough
}

uint8_t Battery::getValue() {
    return _value;
}

int Battery::getPercentage() {
//     Serial.println("***********");
//     Serial.print("145: ");
//     Serial.println(computePercentage(145));

//     Serial.print("134: ");
//     Serial.println(computePercentage(134));

//     Serial.print("125: ");
//     Serial.println(computePercentage(125));

//     Serial.print(getValue());
//     Serial.print(": ");
//     Serial.println(computePercentage(getValue()));

//     Serial.println("***********");
    return computePercentage(getValue());
}

int Battery::computePercentage(uint8_t value) {
    float u = value * 4.20 / 145;       //4.20 is faked a little 
    float p = 123 - (123 / pow(1 + pow(u / 3.7, 80), 0.165));
    int r = floor(p);
    return min(r, 100);
}