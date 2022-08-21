#ifndef rgbled_h
#define rgbled_h

#include <Arduino.h>
#include <FastLED.h>

class RgbLed {
   public:
    RgbLed();

    /**
     * @brief Init function
     */
    void init();

    /**
     * @brief Update callback
     */
    void update();

    /**
     * @brief Set solid light with given color and brightness.
     *
     * @param color color
     * @param brightness brightness 0-255
     */
    void setSolidColor(CRGB color, uint8_t brightness);
    /**
     * @brief Set blinking 300/300ms
     *
     * @param color color 
     * @param brightness brightness
     */

    void setBlinkingColor(CRGB color, uint8_t brightness);

   private:
    enum Visual {
        solid,
        blinking
    };

    CRGB _color;
    uint8_t _brightness;
    Visual _visual;
    uint16_t _callbackCnt;
};

#endif