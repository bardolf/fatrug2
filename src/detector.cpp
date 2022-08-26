#include "detector.h"
#include "logging.h"

#define TIMING_BUDGET 20000
#define RANGE_THRESHOLD 200

Detector::Detector() {
}

bool Detector::init() {
    _lox = new Adafruit_VL53L0X();
    if (_lox->begin()) {
        _lox->setMeasurementTimingBudgetMicroSeconds(TIMING_BUDGET);
        _lox->startRangeContinuous();
        return true;
    }
    return false;
}

void Detector::startMeasurement() {
    _firstMeasurement = true;
    _measurementEnabled = true;
}

void Detector::stopMeasurement() {
    _measurementEnabled = false;
}

void Detector::update() {
    if (!_measurementEnabled) {
        return;
    }
    
    if (_lox->isRangeComplete()) {
        uint16_t range = _lox->readRange();
        bool objectDetected = range > 0 && range <= RANGE_THRESHOLD;

        if (_firstMeasurement) {
            _currObjectDetected = objectDetected;
            _prevObjectDetected = objectDetected;
            _firstMeasurement = false;
        } else {
            _prevObjectDetected = _currObjectDetected;
            _currObjectDetected = objectDetected;
        }
    }
}

bool Detector::isObjectLeft() {
    return _prevObjectDetected && !_currObjectDetected;
}

bool Detector::isObjectArrived() {
    return !_prevObjectDetected && _currObjectDetected;
}
