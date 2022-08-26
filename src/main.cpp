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
#define DEVICE_TYPE 1        // defines whether is it start (0) or finish (1) device

#define RESET_BUTTON_PIN 25  // ext. reset button pin

#define STATE_START 0
#define STATE_READY 1
#define STATE_RUN 2
#define STATE_FINISH 3

#define EVENT_SEND_ERROR 1
#define EVENT_BUTTON_RESET 2
#define EVENT_MESSAGE_INIT 3
#define EVENT_MESSAGE_ACK 4
#define EVENT_MESSAGE_FINISH 5
#define EVENT_DETECTOR_OBJECT_LEFT 6
#define EVENT_DETECTOR_OBJECT_ARRIVED 7

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
uint32_t measuredTime = 0;

QueueHandle_t sendQueue;
QueueHandle_t stateMachineEventQueue;

uint8_t startDeviceAddress[] = {0x08, 0x3A, 0xF2, 0x3A, 0x81, 0xFC};
uint8_t finishDeviceAddress[] = {0x08, 0x3A, 0xF2, 0x3A, 0x81, 0x60};
esp_now_peer_info_t peerInfo;

/**
 * Specifies whether is the start (master) device or not.
 * Usually based on harware configuration.
 */
boolean isStartDevice() {
    return DEVICE_TYPE == 0;
}

void addSendQueue(Message message) {
    if (xQueueSend(sendQueue, &message, 0) != pdTRUE) {
        Log.errorln("Problem while putting a message to a send queue, is it full?");
    }
}

void addStateMachineQueue(Message message) {
    if (xQueueSend(stateMachineEventQueue, &message, 0) != pdTRUE) {
        Log.errorln("Problem while putting a message to a state machine queue, is it full?");
    }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_FAIL) {
        Message message;
        message.event = EVENT_SEND_ERROR;
        addStateMachineQueue(message);
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    Message message;
    memcpy(&message, incomingData, sizeof(message));
    Log.infoln("Received message event %d  and time %d", message.event, message.time);
    addStateMachineQueue(message);
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
                addStateMachineQueue(message);
                addSendQueue(message);
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
        // Log.infoln("Battery value %d", battery.getValue());
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
            case STATE_FINISH:
                display.showTime(measuredTime);
                break;
        }
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}

void stateMachineStartDeviceTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(stateMachineEventQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("SM: state %d, event %d", currentState, message.event);
            if (message.event == EVENT_BUTTON_RESET) {
                currentState = STATE_START;
                detector.stopMeasurement();
                continue;
            }
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
                        Message message;
                        message.event = EVENT_DETECTOR_OBJECT_LEFT;
                        addSendQueue(message);
                        detector.stopMeasurement();
                    }
                    break;
                case STATE_RUN:
                    if (message.event == EVENT_DETECTOR_OBJECT_ARRIVED) {
                        measuredTime = millis() - startTime;
                        message.event = EVENT_MESSAGE_FINISH;
                        message.time = measuredTime;
                        currentState = STATE_FINISH;
                        addSendQueue(message);
                    }
                    break;
                case STATE_FINISH:
                    break;
                default:
                    break;
            }
        }
        Log.infoln("SM: new state %d", currentState);
    }
}

void stateMachineFinishDeviceTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(stateMachineEventQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("SM: state %d, event %d", currentState, message.event);
            if (message.event == EVENT_BUTTON_RESET) {
                currentState = STATE_START;
                detector.stopMeasurement();
                continue;
            }
            switch (currentState) {
                case STATE_START:
                    if (message.event == EVENT_MESSAGE_INIT) {
                        Message message;
                        message.event = EVENT_MESSAGE_ACK;
                        addSendQueue(message);
                        currentState = STATE_READY;
                    }
                    break;
                case STATE_READY:
                    if (message.event == EVENT_DETECTOR_OBJECT_LEFT) {
                        currentState = STATE_RUN;
                        detector.startMeasurement();
                        startTime = millis();
                    }
                    break;
                case STATE_RUN:
                    if (message.event == EVENT_DETECTOR_OBJECT_ARRIVED) {
                        detector.stopMeasurement();
                        addSendQueue(message);
                    } else if (message.event == EVENT_MESSAGE_FINISH) {
                        measuredTime = message.time;
                        currentState = STATE_FINISH;
                    }
                    break;
                case STATE_FINISH:

                    break;
                default:
                    break;
            }
        }
        Log.infoln("SM: new state %d", currentState);
    }
}

void stateMachineTask(void *pvParameters) {
    if (isStartDevice()) {
        stateMachineStartDeviceTask(pvParameters);
    } else {
        stateMachineFinishDeviceTask(pvParameters);
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
            addStateMachineQueue(message);
        }
        if (detector.isObjectLeft()) {
            Log.infoln("Object left.");
            Message message;
            message.event = EVENT_DETECTOR_OBJECT_LEFT;
            addStateMachineQueue(message);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void establishCommunicationTask(void *pvParameters) {
    while (1) {
        if (isStartDevice() && currentState == STATE_START) {
            Message message;
            message.event = EVENT_MESSAGE_INIT;
            Log.infoln("Establishing communication...");
            addSendQueue(message);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void communicationTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(sendQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("Sending message %d from %s", message.event, WiFi.macAddress().c_str());
            uint8_t *address = startDeviceAddress;
            esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *)&message, sizeof(message));
            if (result != ESP_OK) {
                Log.errorln("Error sending the data");
            }
        }
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

    // reset button initialization
    bounce.attach(RESET_BUTTON_PIN, INPUT_PULLUP);
    bounce.interval(10);

    // rgb and display initialization
    rgbLed.init();
    display.init();

    // laser detector initialization
    if (!detector.init()) {
        Log.errorln("Problem with detector initialization.");
    }

    // communication initialization
    WiFi.mode(WIFI_MODE_STA);
    if (esp_now_init() != ESP_OK) {
        Log.errorln("Error initializing ESP-NOW");
        return;
    }
    Log.infoln("MAC Address: %s", WiFi.macAddress().c_str());

    if (isStartDevice()) {
        memcpy(peerInfo.peer_addr, finishDeviceAddress, 6);
    } else {
        memcpy(peerInfo.peer_addr, startDeviceAddress, 6);
    }
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Log.errorln("Failed to add peer");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    xTaskCreatePinnedToCore(updateRgbLedTask, "Upd. RGB", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateBatteryTask, "Upd. battery", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateDisplay, "Upd. display", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(stateMachineTask, "State machine", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(updateDetectorTask, "Upd. detector", 8000, NULL, 4, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(readDetectorTask, "Read detector", 8000, NULL, 4, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(readResetButtonTask, "Reset button", 8000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(communicationTask, "Communication", 8000, NULL, 3, NULL, ARDUINO_RUNNING_CORE);
    if (isStartDevice()) {
        xTaskCreatePinnedToCore(establishCommunicationTask, "Estab. communication", 8000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
    }

    // when started let's reset the peer
    Message message;
    message.event = EVENT_BUTTON_RESET;
    addSendQueue(message);

    // TODO: remove
    rgbLed.setBlinkingColor(CRGB::Red, 50);
}

void loop() {
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // do nothing in loop
}