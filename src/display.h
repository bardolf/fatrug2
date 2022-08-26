#ifndef display_h
#define display_h

#include <Arduino.h>
#include <TM1637Display.h>

class Display {
   public:
    Display();

    /**
     * @brief Init function
     */
    void init();

    void showNumberDec(int num);
    void showTime(uint32_t ms);
    void showZeroTime();

   private:
    TM1637Display* _tm1637;
};

#endif