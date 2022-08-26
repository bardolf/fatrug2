#include <Arduino.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

#include "battery.h"
#include "detector.h"
#include "display.h"
#include "logging.h"
#include "rgbled.h"

// Constants
#define DEVICE_TYPE 0        // defines whether is it start (0) or finish (1) device
#define LASER_THRESHOLD 200  // defines threshold distance in mm

#define RESET_BUTTON_PIN 25  // ext. reset button pin

#define STATE_START 0
#define STATE_READY 1
#define STATE_RUN 2
#define STATE_FINISH 3

#define EVENT_SEND_ERROR 1
#define EVENT_BUTTON_RESET 2
#define EVENT_MESSAGE_ACK 3
#define EVENT_MESSAGE_FINISH 3
#define EVENT_DETECTOR_OBJECT_LEFT 4
#define EVENT_DETECTOR_OBJECT_ARRIVED 5

// Types
typedef struct Message {
    byte event;
    unsigned long time;
} Message;

// Global Variables
unsigned int currentState = STATE_START;
RgbLed rgbLed;
Bounce bounce;
Display display;
Battery battery;
Detector detector;
uint32_t startTime = 0;

QueueHandle_t sendQueue;
QueueHandle_t stateMachineEventQueue;

/**
 * Specifies whether is the start (master) device or not.
 * Usually based on harware configuration.
 */
boolean isStartDevice() {
    return DEVICE_TYPE == 0;
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_FAIL) {
        Message message;
        message.event = EVENT_SEND_ERROR;
        if (xQueueSend(stateMachineEventQueue, &message, 0) != pdTRUE) {
            Log.errorln("Problem while putting a message to a queue, queue is full?");
        }
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    Message message;
    memcpy(&message, incomingData, sizeof(message));
    Log.noticeln("Received message %s - %d - %d", message.event, message.time);
    if (xQueueSend(stateMachineEventQueue, &message, 0) != pdTRUE) {
        Log.errorln("Problem while putting a message to a queue, queue is full?");
    }
}

void readResetButtonTask(void *pvParameters) {
    while (1) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        bounce.update();
        if (bounce.changed()) {
            int deboucedInput = bounce.read();
            if (deboucedInput == HIGH) {
                Message message;
                message.event = EVENT_BUTTON_RESET;
                if (xQueueSend(sendQueue, &message, 0) != pdTRUE) {
                    Log.errorln("Problem while putting a message to a queue, queue is full?");
                }
                if (xQueueSend(stateMachineEventQueue, &message, 0) != pdTRUE) {
                    Log.errorln("Problem while putting a message to a queue, queue is full?");
                }
            }
        }
    }
}

void updateRgbLedTask(void *pvParameters) {
    while (1) {
        rgbLed.update();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void updateBatteryTask(void *pvParameters) {
    while (1) {
        battery.update();
        Log.infoln("Battery value %d", battery.getValue());
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void updateDisplay(void *pvParameters) {
    while (1) {
        switch (currentState) {
            case STATE_START:
                display.showNumberDec(1111);
                break;
            case STATE_READY:
                display.showZeroTime();
                break;
            case STATE_RUN:
                display.showTime(millis() - startTime);
                break;
        }
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}

void stateMachineTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(stateMachineEventQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("SM: event %d", message.event);
            switch (currentState) {
                case STATE_START:
                    if (message.event == EVENT_MESSAGE_ACK) {
                        currentState = STATE_READY;
                        detector.startMeasurement();
                    }
                    break;
                case STATE_READY:
                    if (message.event == EVENT_DETECTOR_OBJECT_LEFT) {
                        currentState = STATE_RUN;
                        startTime = millis();
                    }
                case STATE_RUN:
                    if (message.event == EVENT_BUTTON_RESET) {
                        currentState = STATE_READY;
                    }
                    break;
                default:
                    break;
            }
        }
        Log.infoln("SM: state %d", currentState);
    }
}

void updateDetectorTask(void *pvParameters) {
    while (1) {
        detector.update();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void readDetectorTask(void *pvParameters) {
    while (1) {
        if (detector.isObjectArrived()) {
            Log.infoln("Object arrived.");
            Message message;
            message.event = EVENT_DETECTOR_OBJECT_ARRIVED;
            if (xQueueSend(stateMachineEventQueue, (void *)&message, 0) != pdTRUE) {
                Log.errorln("Problem while putting a message to a queue, queue is full?");
            }
        }
        if (detector.isObjectLeft()) {
            Log.infoln("Object left.");
            Message message;
            message.event = EVENT_DETECTOR_OBJECT_LEFT;            
            if (xQueueSend(stateMachineEventQueue, (void *)&message, 0) != pdTRUE) {
                Log.errorln("Problem while putting a message to a queue, queue is full?");
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);

    // initialize logging
    while (!Serial && !Serial.available()) {
    }
    Log.begin(LOG_LEVEL_INFO, &Serial);
    Log.setPrefix(printPrefix);
    Log.setShowLevel(false);
    Log.infoln("START");

    // initialize queues
    stateMachineEventQueue = xQueueCreate(10, sizeof(Message));
    sendQueue = xQueueCreate(10, sizeof(Message));

    //reset button initialization
    bounce.attach(RESET_BUTTON_PIN, INPUT_PULLUP);
    bounce.interval(10);

    //rgb and display initialization
    rgbLed.init();
    display.init();

    //laser detector initialization
    if (!detector.init()) {
        Log.errorln("Problem with detector initialization.");
    }

    // rgbLed.setSolidColor(CRGB::Blue, 5);
    rgbLed.setBlinkingColor(CRGB::Red, 50);
    xTaskCreatePinnedToCore(updateRgbLedTask, "Upd. RGB Task", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateBatteryTask, "Upd. Battery Task", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateDisplay, "Upd. Display Task", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(stateMachineTask, "State Machine Task", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateDetectorTask, "Upd. Detector Task", 8000, NULL, 4, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(readDetectorTask, "Read Detector Task", 8000, NULL, 4, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(readResetButtonTask, "Reset Button Task", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);

    Message message;
    message.event = EVENT_MESSAGE_ACK;
    if (xQueueSend(stateMachineEventQueue, (void *)&message, 0) != pdTRUE) {
        Log.errorln("Problem while putting a message to a queue, queue is full?");
    }
}

void loop() {
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // do nothing in loop
}