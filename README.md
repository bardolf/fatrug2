# FATRUG2

Fully automatic time project ([FAT](https://en.wikipedia.org/wiki/Fully_automatic_time)) , based on [FireBeetle 2 ESP32-E IoT Microcontroller](https://www.dfrobot.com/product-2195.html). 

## Electronics

Schema and PCB drawn in KiCAD and files are part of the project. Peripherials are following. 

- MCU
![ESP32-E pinout](doc/img/01_esp32-e-pinout.png)

- laser distance sensor - GY-VL53L0X I2, connected via connector labeled _laser_ (SDA, SCL, 3.3V, GND)

![GY-VL53L0X](doc/img/03_laser_GY-VL53L0X_I2C.png)

- TM1637 4-Digit LED 0.56" 0.56inch 7 Segments Display, connected via connector labeled _7seg_

![TM1637](doc/img/04_TM1637.png)

- RGB LED NeoPixel WS2812 8mm, connected via connector labeled _RGB LED_

![WS2812](doc/img/05_rgb_led.png)

- button, connected to ground and PIN D2 (25) 

![button](doc/img/06_button.png)

- LiPol Batterry 104050 2500mAh 3.7V JST-PH 2.0

![battery](doc/img/07_battery.png)


- 2x resistor 20k - for battery capacity measurement on PIN A0 (36)

- some JST-PH connectors, DUPONT connectors

If you make your own BOM don't forget to [multiply it by two](https://youtube.com/clip/UgkxrPXdP5GEu_9O2kQU0yOqQVvTavGrz9Ro) - you need 2 boxes.


### PCB

![PCB](doc/img/02_pcb_view.png)


## Box

![Box](doc/img/08_box.png)

## State Diagram 


## Photos

