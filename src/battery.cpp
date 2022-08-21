#include "battery.h"

#define ANALOG_BATTERY_PIN 36

Battery::Battery() {
}

void Battery::update() {
    _value = analogRead(ANALOG_BATTERY_PIN) >> 4;   //8bits more than enough
}

uint16_t Battery::getValue() {
    return _value;
} 