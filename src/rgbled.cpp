#include "rgbled.h"
#define RGB_LED_PIN 2

RgbLed::RgbLed() {
    _color = CRGB::Black;
}

void RgbLed::init() {
    FastLED.addLeds<WS2812Controller800Khz, RGB_LED_PIN>(&_color, 1);
}

void RgbLed::setSolidColor(CRGB color, uint8_t brightness) {
    _visual = solid;
    _color = color;
    _brightness = brightness;
}

void RgbLed::setBlinkingColor(CRGB color, uint8_t brightness) {
    _visual = blinking;
    _color = color;
    _brightness = brightness;
}

void RgbLed::update() {
    if (_visual == solid) {
        FastLED.show();
        FastLED.setBrightness(_brightness);
    } else if (_visual == blinking) {
        if (_callbackCnt < 6) {            
            FastLED.setBrightness(_brightness);
        } else {            
            FastLED.setBrightness(0);
        }
        FastLED.show();
        if (_callbackCnt > 12) {
            _callbackCnt = 0;
        } else {
            _callbackCnt++;
        }
    }
}