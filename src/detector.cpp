#include "detector.h"

#include "logging.h"

Detector::Detector() {
    _detectorPulseInTimeout = 2 * (unsigned long)((float)RANGE_THRESHOLD_CM / (float)SOUND_SPEED_HALF);
}

void Detector::startMeasurement() {
    _prevObjectDetected = false;
    _measurementEnabled = true;    
    _prevPrevDistance = _prevDistance = _distance = 0;
}

void Detector::stopMeasurement() {
    _measurementEnabled = false;
}

void Detector::init() {
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT_PULLDOWN);
}

uint32_t Detector::getCompensationTime() {
    return _time - _prevPrevTime;
}

float Detector::measureDistance() {
    digitalWrite(TRIGGER_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(TRIGGER_PIN, LOW);
    // Serial.print(millis());
    // Serial.print("ms ");
    long duration = pulseIn(ECHO_PIN, HIGH, _detectorPulseInTimeout);
    // Serial.print(millis());
    // Serial.print("ms ~ ");
    // Serial.println(duration);
    if (duration == 0) {  // pulseIn timeout
        return 10 * RANGE_THRESHOLD_CM;
    }
    return duration * SOUND_SPEED_HALF;
}

DetectedObjectState Detector::read() {
    if (_measurementEnabled) {
        _prevPrevDistance = _prevDistance;
        _prevDistance = _distance;
        _distance = measureDistance();
      
        _prevPrevTime = _prevTime;
        _prevTime = _time;
        _time = millis();

        if (_distance > RANGE_THRESHOLD_CM && _prevDistance > RANGE_THRESHOLD_CM && _prevPrevDistance > RANGE_THRESHOLD_CM && _prevObjectDetected) {
            _prevObjectDetected = false;
            Log.infoln("LEFT %F cm (%Fcm, %Fcm)", _distance, _prevDistance, _prevPrevDistance);
            return LEFT;
        } else if (_distance <= RANGE_THRESHOLD_CM && _prevDistance <= RANGE_THRESHOLD_CM && _prevPrevDistance <= RANGE_THRESHOLD_CM &&
                   abs(1 - _distance / _prevDistance) < 0.05 && abs(1 - _prevPrevDistance / _prevPrevDistance) < 0.05 && !_prevObjectDetected) {
            _prevObjectDetected = true;
            Log.infoln("ARRIVED %F cm (%Fcm, %Fcm)", _distance, _prevDistance, _prevPrevDistance);
            return ARRIVED;
        }
    }
    return NONE;
}
