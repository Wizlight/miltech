#include <Arduino.h>
#include <RadioLib.h>

const int BUTTON_PIN = 25;

SX1276 radio = new Module(
    18,
    26,
    23,
    RADIOLIB_NC
);

enum ButtonEvent {
    BUTTON_SINGLE,
    BUTTON_DOUBLE
};

struct ButtonMessage {
    ButtonEvent event;
    unsigned long pressTime;
};

QueueHandle_t buttonQueue;
QueueHandle_t radioQueue;


void buttonTask(void *parameter) {
    while (true) {

    
        if (digitalRead(BUTTON_PIN) == LOW) {
            unsigned long pressTime = millis();

            while (digitalRead(BUTTON_PIN) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            unsigned long startTime = millis();
            bool secondPress = false;

            while (millis() - startTime < 300) {

                if (digitalRead(BUTTON_PIN) == LOW) {
                    secondPress = true;

                    while (digitalRead(BUTTON_PIN) == LOW) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }

            ButtonEvent event;

            if (secondPress) {
                event = BUTTON_DOUBLE;
            } else {
                event = BUTTON_SINGLE;
            }

            ButtonMessage message;

            message.event = event;
            message.pressTime = pressTime;

            xQueueSend(
                buttonQueue,
                &message,
                portMAX_DELAY
            );

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void radioTask(void *parameter) {

    ButtonMessage message;

    while (true) {

        if (xQueueReceive(
                radioQueue,
                &message,
                portMAX_DELAY
            ) == pdTRUE) {

            if (message.event == BUTTON_SINGLE) {

                Serial.println("Radio Task: sending BUTTON SINGLE");

                int state = radio.transmit("BUTTON SINGLE");

                if (state == RADIOLIB_ERR_NONE) {
                    unsigned long elapsedTime = millis() - message.pressTime;

                    Serial.print("Time from button press to radio sent: ");
                    Serial.print(elapsedTime);
                    Serial.println(" ms");
                } else {
                    Serial.print("Radio error: ");
                    Serial.println(state);
                }
            }

            if (message.event == BUTTON_DOUBLE) {

                Serial.println("Radio Task: sending BUTTON DOUBLE");

                int state = radio.transmit("BUTTON DOUBLE");

                if (state == RADIOLIB_ERR_NONE) {
                    unsigned long elapsedTime = millis() - message.pressTime;

                    Serial.print("Time from button press to radio sent: ");
                    Serial.print(elapsedTime);
                    Serial.println(" ms");
                } else {
                    Serial.print("Radio error: ");
                    Serial.println(state);
                }
            }
        }
    }
}


void setup() {
    Serial.begin(115200);
    SPI.begin(5, 19, 27, 18);
    int state = radio.begin(867.5);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Radio initialized");
    } else {
        Serial.print("Radio init failed: ");
        Serial.println(state);
    }

    delay(1000);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    buttonQueue = xQueueCreate(
        5,
        sizeof(ButtonMessage)
    );

    radioQueue = xQueueCreate(
        5,
        sizeof(ButtonMessage)
    );

    xTaskCreate(
        buttonTask,
        "Button Task",
        2048,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        radioTask,
        "Radio Task",
        2048,
        NULL,
        1,
        NULL
    );
}


void loop() {

    ButtonMessage message;

    if (xQueueReceive(
            buttonQueue,
            &message,
            portMAX_DELAY
        ) == pdTRUE) {

        if (message.event == BUTTON_SINGLE) {
            Serial.println("Loop received: SINGLE");
        }

        if (message.event == BUTTON_DOUBLE) {
            Serial.println("Loop received: DOUBLE");
        }

        xQueueSend(
            radioQueue,
            &message,
            portMAX_DELAY
        );
    }
}