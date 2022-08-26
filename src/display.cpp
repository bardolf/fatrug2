#include "display.h"

#define CLK 14
#define DIO 13

Display::Display() {
}

void Display::init() {
    _tm1637 = new TM1637Display(CLK, DIO);
    _tm1637->setBrightness(7);
}

void Display::showNumberDec(int num) {
    _tm1637->showNumberDec(num);
}

void Display::showZeroTime() {
   _tm1637->showNumberDecEx(0, 0b1000000, true); 
}

void Display::showTime(uint32_t millis) {
    int divider = 1;    
    // less than 99.99s    
    if (millis <= 59999) {
        _tm1637->showNumberDecEx(millis / 10, 0b01000000, true);
    } else {
        uint32_t mins = millis / 1000 / 60;
        uint32_t secs = millis / 1000 - mins * 60;
        _tm1637->showNumberDecEx(mins * 100 + secs, 0b01000000, true);
    }
}
