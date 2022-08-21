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

    /**
     * @brief Update callback
     */
    void update();

   private:
    TM1637Display* _tm1637;
};

#endif