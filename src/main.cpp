#include <Arduino.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

#include "logging.h"
#include "rgbled.h"

// Constants
#define DEVICE_TYPE 0  // defines whether is it start (0) or finish (1) device

#define RESET_BUTTON_PIN 27  // onboard button
#define SEND_QUEUE_LENGTH 5
#define STATE_MACHINE_EVENT_QUEUE_LENGTH 5

#define STATE_START 0
#define STATE_READY 1
#define STATE_RUN 2
#define STATE_FINISH 3

#define EVENT_SEND_ERROR 1
#define EVENT_BUTTON_RESET 2
#define EVENT_MESSAGE_ACK 3
#define EVENT_MESSAGE_FINISH 3

// Types
typedef struct Message {
    byte event;
    unsigned long time;
} Message;

// Global Variables
unsigned int currentState = STATE_START;
RgbLed rgbLed;
Bounce bounce = Bounce();
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

// Tasks
void startStateMachineTask(void *pvParameters) {
    switch (currentState) {
        case STATE_START:

            break;
    }
}

void readResetButtonTask(void *pvParameters) {
    while (1) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        bounce.update();
        if (bounce.changed()) {
            int deboucedInput = bounce.read();
            if (deboucedInput == LOW) {
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

void updateTgbLedTask(void *pvParameters) {    
    while (1) {
        rgbLed.update();        
        vTaskDelay(10 / portTICK_PERIOD_MS);        
    }
}

void setup() {
    Serial.begin(115200);
    rgbLed.init();

    // rgbLed.setSolidColor(CRGB::Blue, 5);
    rgbLed.setBlinkingColor(CRGB::Red, 50);
    xTaskCreatePinnedToCore(updateTgbLedTask, "Update RGB LED Task", 1200, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
}

void loop() {
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // do nothing in loop
}