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
#define DEVICE_TYPE 0  // defines whether is it start (0) or finish (1) device

#define RESET_BUTTON_PIN 0  // ext. reset button pin

typedef enum {
    STATE_UNKNOWN,
    STATE_START,
    STATE_READY,
    STATE_RUN,
    STATE_FINISH
} State;

// used for logging/debuggin purposes
const char *stateName(State state) {
    static char const *stateNames[5] = {"STATE_UNKNOWN", "STATE_START", "STATE_READY", "STATE_RUN", "STATE_FINISH"};
    if (state >= 0 && state < 5) {
        return stateNames[state];
    } else {
        return "UNDEFINED";
    }
}

typedef enum {
    EVENT_SEND_ERROR,
    EVENT_BUTTON_RESET,
    EVENT_MESSAGE_INIT,
    EVENT_MESSAGE_ACK,
    EVENT_MESSAGE_FINISH,
    EVENT_DETECTOR_OBJECT_LEFT,
    EVENT_DETECTOR_OBJECT_ARRIVED
} Event;

// used for logging/debuggin purposes
const char *eventName(Event event) {
    static char const *eventNames[7] = {"EVENT_SEND_ERROR", "EVENT_BUTTON_RESET", "EVENT_MESSAGE_INIT", "EVENT_MESSAGE_ACK", "EVENT_MESSAGE_FINISH", "EVENT_DETECTOR_OBJECT_LEFT", "EVENT_DETECTOR_OBJECT_ARRIVED"};
    if (event >= 0 && event < 7) {
        return eventNames[event];
    } else {
        return "UNDEFINED";
    }
}

// Types
typedef struct Message {
    Event event;
    unsigned long time;
} Message;

// Global Variables
State currentState = STATE_UNKNOWN;
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
    Log.infoln("Received message event %s  and time %d", eventName(message.event), message.time);
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
        Log.infoln("Battery value %d prct", battery.getPercentage());
        vTaskDelay(1000 * 60 / portTICK_PERIOD_MS);
    }
}

void updateDisplay(void *pvParameters) {
    while (1) {
        display.update();
        vTaskDelay(25 / portTICK_PERIOD_MS);
    }
}

void stateMachineStartDeviceTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(stateMachineEventQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("SM: state %s, event %s", stateName(currentState), eventName(message.event));
            if (message.event == EVENT_BUTTON_RESET) {
                currentState = STATE_START;
                display.showConnecting();
                detector.stopMeasurement();
                continue;
            }
            switch (currentState) {
                case STATE_START:
                    if (message.event == EVENT_MESSAGE_ACK) {
                        display.showZeroTime();
                        currentState = STATE_READY;
                        detector.startMeasurement();
                    }
                    break;
                case STATE_READY:
                    if (message.event == EVENT_DETECTOR_OBJECT_LEFT) {
                        display.showTimeContinuously();
                        detector.stopMeasurement();
                        currentState = STATE_RUN;
                        startTime = millis();
                        Message message;
                        message.event = EVENT_DETECTOR_OBJECT_LEFT;
                        addSendQueue(message);
                    }
                    break;
                case STATE_RUN:
                    if (message.event == EVENT_DETECTOR_OBJECT_ARRIVED) {
                        measuredTime = millis() - startTime;
                        display.showTime(measuredTime);
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
        Log.infoln("SM: new state %s", stateName(currentState));
    }
}

void stateMachineFinishDeviceTask(void *pvParameters) {
    Message message;
    while (1) {
        if (xQueueReceive(stateMachineEventQueue, (void *)&message, 10000) == pdTRUE) {
            Log.infoln("SM: state %s, event %s", stateName(currentState), eventName(message.event));
            if (message.event == EVENT_BUTTON_RESET) {
                currentState = STATE_START;
                detector.stopMeasurement();
                display.showConnecting();
                continue;
            }
            switch (currentState) {
                case STATE_START:
                    display.showZeroTime();
                    if (message.event == EVENT_MESSAGE_INIT) {
                        Message message;
                        message.event = EVENT_MESSAGE_ACK;
                        addSendQueue(message);
                        currentState = STATE_READY;
                    }
                    break;
                case STATE_READY:
                    display.showZeroTime();
                    if (message.event == EVENT_DETECTOR_OBJECT_LEFT) {
                        display.showTimeContinuously();
                        currentState = STATE_RUN;
                        detector.startMeasurement();
                        startTime = millis();
                    }
                    break;
                case STATE_RUN:
                    display.showTimeContinuously();
                    if (message.event == EVENT_DETECTOR_OBJECT_ARRIVED) {
                        detector.stopMeasurement();
                        addSendQueue(message);
                    } else if (message.event == EVENT_MESSAGE_FINISH) {
                        measuredTime = message.time;
                        display.showTime(measuredTime);
                        currentState = STATE_FINISH;
                    }
                    break;
                case STATE_FINISH:

                    break;
                default:
                    break;
            }
        }
        Log.infoln("SM: new state %s", stateName(currentState));
    }
}

void stateMachineTask(void *pvParameters) {
    if (isStartDevice()) {
        stateMachineStartDeviceTask(pvParameters);
    } else {
        stateMachineFinishDeviceTask(pvParameters);
    }
}

void readDetectorTask(void *pvParameters) {
    while (1) {
        DetectedObjectState detectedObjectState = detector.read();
        if (detectedObjectState == ARRIVED) {
            Message message;
            message.event = EVENT_DETECTOR_OBJECT_ARRIVED;
            addStateMachineQueue(message);
            Log.infoln("Object arrived");
        } else if (detectedObjectState == LEFT) {
            Message message;
            message.event = EVENT_DETECTOR_OBJECT_LEFT;
            addStateMachineQueue(message);
            Log.infoln("Object left");
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
            Log.infoln("Sending message %s from %s", eventName(message.event), WiFi.macAddress().c_str());
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

    // detector initialization
    detector.init();

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
    xTaskCreatePinnedToCore(readDetectorTask, "Read detector", 8000, NULL, 6, NULL, ARDUINO_RUNNING_CORE);
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
    rgbLed.setBlinkingColor(CRGB::Green, 15);

    // start the SM
    currentState = STATE_START;
    display.showConnecting();
}

void loop() {
    vTaskDelay(10000 / portTICK_PERIOD_MS);  // do nothing in loop
}