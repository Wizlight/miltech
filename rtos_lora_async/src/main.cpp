#include <Arduino.h>
#include <RadioLib.h>

const int LED_PIN = 25;

SX1276 radio = new Module(
    18,
    26,
    23,
    RADIOLIB_NC
);

volatile bool transmitted = false;

void onPacketSent() {
    transmitted = true;
}

void ledTask(void *parameter) {
    while (true) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));

        Serial.print("LED: ");
        Serial.println(millis());

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void radioTask(void *parameter) {

    uint8_t packet[32];
    memset(packet, 'A', sizeof(packet));

    while (true) {

        transmitted = false;

        Serial.print("TX START: ");
        Serial.println(millis());

        int state = radio.startTransmit(
            packet,
            sizeof(packet)
        );

        if (state != RADIOLIB_ERR_NONE) {
            Serial.print("TX error: ");
            Serial.println(state);
        }

        while (!transmitted) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        radio.finishTransmit();

        Serial.print("TX DONE: ");
        Serial.println(millis());

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);

    SPI.begin(5, 19, 27, 18);

    int state = radio.begin(867.5);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("Radio error: ");
        Serial.println(state);
        while (true) {}
    }

    radio.setSpreadingFactor(11);

    radio.setPacketSentAction(onPacketSent);

    xTaskCreatePinnedToCore(
        ledTask,
        "LED Task",
        2048,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        radioTask,
        "Radio Task",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}

void loop() {
}