#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

uint8_t packet[64];

void setup() {
    Serial.begin(115200);

    SPI.begin(5, 19, 27, 18);

    int state = radio.beginFSK(915.0, 100.0, 100.0);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("FSK initialized!");
    } else {
        Serial.print("FSK init failed: ");
        Serial.println(state);
        while (true);
    }

    state = radio.fixedPacketLengthMode(64);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("Fixed packet mode failed: ");
        Serial.println(state);
        while (true);
    }

    Serial.println("Waiting for packets...");
}

void loop() {
    int state = radio.receive(packet, sizeof(packet), 2000);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("Received: ");
        Serial.write(packet, sizeof(packet));
        Serial.println();

        Serial.print("RSSI: ");
        Serial.println(radio.getRSSI());
    } else {
        Serial.print("Receive failed, code: ");
        Serial.println(state);
    }
}