#include "detector.h"

Detector::Detector() {
}

void Detector::startMeasurement() {
    _prevObjectDetected = false;
    _measurementEnabled = true;
}

void Detector::stopMeasurement() {
    _measurementEnabled = false;
}

void Detector::init() {
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT_PULLDOWN);
}

float Detector::measureDistance() {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH);
    return duration * SOUND_SPEED_HALF;
}

DetectedObjectState Detector::read() {
    if (_measurementEnabled) {
        float distance = measureDistance();
        if (distance > RANGE_THRESHOLD_CM && _prevDistance > RANGE_THRESHOLD_CM && _prevObjectDetected) {
            _prevObjectDetected = false;
            // Serial.println(distance);
            return LEFT;
        } else if (distance <= RANGE_THRESHOLD_CM && abs(1 - distance / _prevDistance) < 0.1 && !_prevObjectDetected) {
            _prevObjectDetected = true;
            // Serial.println(distance);
            return ARRIVED;
        }
        _prevDistance = distance;
    }
    return NONE;
}
