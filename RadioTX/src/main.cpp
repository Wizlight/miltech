#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);
uint8_t packet[64];
int counter = 0;

void setup() {
    Serial.begin(115200);

    SPI.begin(5, 19, 27, 18);
    memset(packet, 'A', sizeof(packet));
    int state = radio.begin(915.0, 500.0, 12);
    radio.implicitHeader(64);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa initialized!");
    } else {
        Serial.print("LoRa init failed: ");
        Serial.println(state);
        while (true);
    }
}

void loop() {
    Serial.write(packet, sizeof(packet));
    Serial.println();

    unsigned long start = micros();
    int state = radio.transmit(packet, sizeof(packet));
    unsigned long finish = micros();
    unsigned long duration = finish - start;

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Sent successfully");
        Serial.println("Transmission time: " + String(duration / 1000000.0, 3) + " s");
    } else {
        Serial.print("Send failed: ");
        Serial.println(state);
    }

    delay(1000);
}