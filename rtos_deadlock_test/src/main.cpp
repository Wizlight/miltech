#include <Arduino.h>

SemaphoreHandle_t mutexA;
SemaphoreHandle_t mutexB;

void taskA(void *parameter) {

    xSemaphoreTake(mutexA, portMAX_DELAY);

    Serial.println("Task A: mutex A taken");

    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.println("Task A: waiting for mutex B");

    while (xSemaphoreTake(mutexB, 0) != pdTRUE) {

    }
}

void taskB(void *parameter) {

    xSemaphoreTake(mutexB, portMAX_DELAY);

    Serial.println("Task B: mutex B taken");

    vTaskDelay(pdMS_TO_TICKS(100));

    Serial.println("Task B: waiting for mutex A");

    while (xSemaphoreTake(mutexA, 0) != pdTRUE) {

    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    mutexA = xSemaphoreCreateMutex();
    mutexB = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
        taskA,
        "Task A",
        2048,
        NULL,
        2,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        taskB,
        "Task B",
        2048,
        NULL,
        2,
        NULL,
        1
    );
}

void loop() {
}