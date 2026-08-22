#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

void setup() {
    Serial.begin(115200);

    SPI.begin(5, 19, 27, 18);

    int state = radio.begin(915.0, 500.0, 12);
    radio.implicitHeader(64);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa initialized!");
    } else {
        Serial.print("LoRa init failed: ");
        Serial.println(state);
        while (true);
    }

    Serial.println("Waiting for packets...");
}

void loop() {
    String message;

    int state = radio.receive(message);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("Received: ");
        Serial.println(message);

        Serial.print("RSSI: ");
        Serial.println(radio.getRSSI());

        Serial.print("SNR: ");
        Serial.println(radio.getSNR());
    }
}