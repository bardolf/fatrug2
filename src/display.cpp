#include "display.h"

#define CLK 14
#define DIO 13

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D
const uint8_t digitToSegment[] PROGMEM = {
    // XGFEDCBA
    0b00111111,  // 0
    0b00000110,  // 1
    0b01011011,  // 2
    0b01001111,  // 3
    0b01100110,  // 4
    0b01101101,  // 5
    0b01111101,  // 6
    0b00000111,  // 7
    0b01111111,  // 8
    0b01101111,  // 9
    0b01110111,  // A
    0b01111100,  // b
    0b00111001,  // C
    0b01011110,  // d
    0b01111001,  // E
    0b01110001   // F
};

// ASCII Map - Index 0 starts at ASCII 32
const uint8_t asciiToSegment[] PROGMEM = {
    0b00000000,  // 032 (Space)
    0b00110000,  // 033 !
    0b00100010,  // 034 "
    0b01000001,  // 035 #
    0b01101101,  // 036 $
    0b01010010,  // 037 %
    0b01111100,  // 038 &
    0b00100000,  // 039 '
    0b00111001,  // 040 (
    0b00001111,  // 041 )
    0b00100001,  // 042 *
    0b01110000,  // 043 +
    0b00001000,  // 044 ,
    0b01000000,  // 045 -
    0b00001000,  // 046 .
    0b01010010,  // 047 /
    0b00111111,  // 048 0
    0b00000110,  // 049 1
    0b01011011,  // 050 2
    0b01001111,  // 051 3
    0b01100110,  // 052 4
    0b01101101,  // 053 5
    0b01111101,  // 054 6
    0b00000111,  // 055 7
    0b01111111,  // 056 8
    0b01101111,  // 057 9
    0b01001000,  // 058 :
    0b01001000,  // 059 ;
    0b00111001,  // 060 <
    0b01001000,  // 061 =
    0b00001111,  // 062 >
    0b01010011,  // 063 ?
    0b01011111,  // 064 @
    0b01110111,  // 065 A
    0b01111100,  // 066 B
    0b00111001,  // 067 C
    0b01011110,  // 068 D
    0b01111001,  // 069 E
    0b01110001,  // 070 F
    0b00111101,  // 071 G
    0b01110110,  // 072 H
    0b00000110,  // 073 I
    0b00011110,  // 074 J
    0b01110110,  // 075 K
    0b00111000,  // 076 L
    0b00010101,  // 077 M
    0b00110111,  // 078 N
    0b00111111,  // 079 O
    0b01110011,  // 080 P
    0b01100111,  // 081 Q
    0b00110001,  // 082 R
    0b01101101,  // 083 S
    0b01111000,  // 084 T
    0b00111110,  // 085 U
    0b00011100,  // 086 V
    0b00101010,  // 087 W
    0b01110110,  // 088 X
    0b01101110,  // 089 Y
    0b01011011,  // 090 Z
    0b00111001,  // 091 [
    0b01100100,  // 092 (backslash)
    0b00001111,  // 093 ]
    0b00100011,  // 094 ^
    0b00001000,  // 095 _
    0b00100000,  // 096 `
    0b01110111,  // 097 a
    0b01111100,  // 098 b
    0b01011000,  // 099 c
    0b01011110,  // 100 d
    0b01111001,  // 101 e
    0b01110001,  // 102 f
    0b01101111,  // 103 g
    0b01110100,  // 104 h
    0b00000100,  // 105 i
    0b00011110,  // 106 j
    0b01110110,  // 107 k
    0b00011000,  // 108 l
    0b00010101,  // 109 m
    0b01010100,  // 110 n
    0b01011100,  // 111 o
    0b01110011,  // 112 p
    0b01100111,  // 113 q
    0b01010000,  // 114 r
    0b01101101,  // 115 s
    0b01111000,  // 116 t
    0b00011100,  // 117 u
    0b00011100,  // 118 v
    0b00101010,  // 119 w
    0b01110110,  // 120 x
    0b01101110,  // 121 y
    0b01011011,  // 122 z
    0b00111001,  // 123 {
    0b00110000,  // 124 |
    0b00001111,  // 125 }
    0b01000000,  // 126 ~
    0b00000000   // 127 
};

// Data from Animator Tool
const uint8_t CONNECTING[11][4] = {
    {0x39, 0x3f, 0x37, 0x37},  // CONN
    {0x3f, 0x37, 0x37, 0x79},  // ONNE
    {0x37, 0x37, 0x79, 0x39},  // NNEC
    {0x37, 0x79, 0x39, 0x31},  // NECT
    {0x79, 0x39, 0x31, 0x30},  // ECTI
    {0x39, 0x31, 0x30, 0x37},  // CTIN
    {0x31, 0x30, 0x37, 0x3d},  // TING
    {0x30, 0x37, 0x3d, 0x00},  // ING
    {0x3f, 0x37, 0x00, 0x39},  // NG C
    {0x37, 0x00, 0x39, 0x3f},  // G CO
    {0x0, 0x39, 0x3f, 0x37}   //  CON
};

Display::Display() {
}

void Display::init() {
    _tm1637 = new TM1637Display(CLK, DIO);
    _tm1637->setBrightness(0x0f);
}

void Display::showNumber(uint16_t number) {
    _mode = NUMBER;
    _number = number;
}

void Display::showZeroTime() {
    _mode = ZERO_TIME;
}

void Display::showTimeContinuously() {
    _mode = CONTINUOUS_TIME;
    _startTime = millis();
}

void Display::showTime(uint32_t time) {
    _mode = TIME;
    _time = time;
}

void Display::showTimeInternal(uint32_t time) {
    int divider = 1;
    // less than 99.99s
    if (time <= 59999) {
        _tm1637->showNumberDecEx(time / 10, 0b01000000, true);
    } else {
        uint32_t mins = time / 1000 / 60;
        uint32_t secs = time / 1000 - mins * 60;
        _tm1637->showNumberDecEx(mins * 100 + secs, 0b01000000, true);
    }
}

void Display::showConnecting() {
    _mode = ANIMATION;
    _frames = CONNECTING;
    _framesCount = sizeof(CONNECTING) / 4;
    _frameIdx = 0;
    _frameDelay = 350;
    _nextFrameMillis = millis() + _frameDelay;
}

void Display::update() {
    switch (_mode) {
        case ZERO_TIME:
            _tm1637->showNumberDecEx(0, 0b1000000, true);
            break;
        case NUMBER:
            _tm1637->showNumberDec(_number);
            break;
        case CONTINUOUS_TIME:
            showTimeInternal(millis() - _startTime);
            break;
        case TIME:
            showTimeInternal(_time);
            break;
        case ANIMATION:
            _tm1637->setSegments(_frames[_frameIdx]);
            if (millis() >= _nextFrameMillis) {
                _nextFrameMillis = _nextFrameMillis + _frameDelay;
                _frameIdx = (_frameIdx + 1) % _framesCount;
            }
        default:
            break;
    }
}
