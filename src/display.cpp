#include "display.h"

#define CLK 14
#define DIO 13

Display::Display() {
}

void Display::init() {
    _tm1637 = new TM1637Display(CLK, DIO);
    _tm1637->setBrightness(7);
}

void Display::update() {
    _tm1637->showNumberDecEx(9876);
}