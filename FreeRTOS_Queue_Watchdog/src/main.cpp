#include <Arduino.h>

const int BUTTON_PIN = 32;
const int LED_PIN = 23;

QueueHandle_t intervalQueue;

void buttonTask(void *parameter) {

    int intervals[] = {250, 500, 1000, 2000};
    int index = 0;

    while (true) {

        if (digitalRead(BUTTON_PIN) == LOW) {

            // Перше натискання: чекаємо відпускання
            while (digitalRead(BUTTON_PIN) == LOW) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            // Даємо 300 мс на друге натискання
            unsigned long startTime = millis();
            bool secondPress = false;

            while (millis() - startTime < 300) {

                if (digitalRead(BUTTON_PIN) == LOW) {
                    secondPress = true;

                    // Чекаємо відпускання другого натискання
                    while (digitalRead(BUTTON_PIN) == LOW) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }

            if (secondPress) {
                index--;

                if (index < 0) {
                    index = 3;
                }

                Serial.print("DOUBLE: ");
            } else {
                index++;

                if (index > 3) {
                    index = 0;
                }

                Serial.print("SINGLE: ");
            }

            int newInterval = intervals[index];

            xQueueSend(
                intervalQueue,
                &newInterval,
                portMAX_DELAY
            );

            Serial.print("new interval = ");
            Serial.print(newInterval);
            Serial.print(" ms, Core ");
            Serial.println(xPortGetCoreID());

            vTaskDelay(pdMS_TO_TICKS(50));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ledTask(void *parameter) {

    int interval = 250;
    bool ledState = false;

    while (true) {

        int newInterval;

        if (xQueueReceive(
                intervalQueue,
                &newInterval,
                pdMS_TO_TICKS(interval)
            ) == pdTRUE) {

            interval = newInterval;

            Serial.print("LED Task, Core ");
            Serial.print(xPortGetCoreID());
            Serial.print(": received interval = ");
            Serial.println(interval);

        } else {

            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    intervalQueue = xQueueCreate(
        5,
        sizeof(int)
    );

    xTaskCreatePinnedToCore(
        buttonTask,
        "Button Task",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        ledTask,
        "LED Task",
        2048,
        NULL,
        1,
        NULL,
        1
    );
}

void loop() {
}