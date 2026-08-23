#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

uint8_t packet[64];
unsigned long counter = 0;

void setup() {
    Serial.begin(115200);

    SPI.begin(5, 19, 27, 18);

    int state = radio.begin(867.5, 125.0, 6);
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
    counter++;
    
    memset(packet, ' ', sizeof(packet));

    snprintf((char*)packet, sizeof(packet), "PACKET:%lu", counter);

    Serial.print("Sending: PACKET:");
    Serial.println(counter);

    unsigned long start = micros();

    int state = radio.transmit(packet, sizeof(packet));

    unsigned long duration = micros() - start;

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Sent successfully");
        Serial.println(
            "Transmission time: " +
            String(duration / 1000000.0, 3) +
            " s"
        );
    } else {
        Serial.print("Send failed: ");
        Serial.println(state);
    }

    delay(5000);
}