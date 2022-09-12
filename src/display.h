#ifndef display_h
#define display_h

#include <Arduino.h>
#include <TM1637Display.h>

typedef enum {
    NUMBER,
    CONTINUOUS_TIME,
    TIME,
    ANIMATION,
    ZERO_TIME
} Mode;

class Display {
   public:
    Display();
    void init();
    void update();

    void showNumber(uint16_t num);
    void showTimeContinuously(uint32_t time);
    void showTime(uint32_t time);
    void showZeroTime();        
    void showConnecting();
    void showError();

   private:   
    TM1637Display* _tm1637;
    Mode _mode;    
    uint16_t _number;
    uint32_t _startTime;
    uint32_t _time;

    uint8_t _frameIdx;    
    uint32_t _nextFrameMillis;
    const uint8_t (*_frames)[4];
    uint8_t _framesCount;
    uint16_t _frameDelay;

    void showTimeInternal(uint32_t time);
};

#endif