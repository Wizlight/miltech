#include <Arduino.h>
#include "esp_flash.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    uint32_t address = 0x8000;
    uint8_t buffer[16];

    Serial.println("Partition table raw data:");

    for (int offset = 0; offset < 0xC00; offset += 16) {

        esp_err_t result = esp_flash_read(
            NULL,
            buffer,
            address + offset,
            16
        );

        if (result != ESP_OK) {
            Serial.println("Flash read error");
            return;
        }

        Serial.print("0x");
        Serial.print(address + offset, HEX);
        Serial.print(": ");

        for (int i = 0; i < 16; i++) {
            if (buffer[i] < 0x10) {
                Serial.print("0");
            }

            Serial.print(buffer[i], HEX);
            Serial.print(" ");
        }

        Serial.println();
    }
}

void loop() {
}