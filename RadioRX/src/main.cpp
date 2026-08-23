#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <U8g2lib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE
);

uint8_t packet[64];

unsigned long lastPacketNumber = 0;
unsigned long receivedPackets = 0;
unsigned long lostPackets = 0;
unsigned long lastPacketTime = 0;

bool firstPacket = true;
bool linkLost = false;

float lastRSSI = 0;
float lastSNR = 0;

void updateDisplay() {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);

    display.setCursor(0, 10);
    display.print(linkLost ? "LINK LOST" : "LINK OK");

    display.setCursor(0, 23);
    display.print("PKT: ");
    display.print(lastPacketNumber);

    display.setCursor(0, 34);
    display.print("RSSI: ");
    display.print(lastRSSI, 1);

    display.setCursor(0, 45);
    display.print("SNR: ");
    display.print(lastSNR, 1);

    display.setCursor(0, 56);
    display.print("RX:");
    display.print(receivedPackets);
    display.print(" LOST:");
    display.print(lostPackets);

    display.sendBuffer();
}

void setup() {
    Serial.begin(115200);

    Wire.begin(21, 22);

    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(0, 12, "Starting...");
    display.sendBuffer();

    SPI.begin(5, 19, 27, 18);

    int state = radio.begin(867.5, 125.0, 6);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa init failed: ");
        Serial.println(state);
        while (true);
    }

    state = radio.implicitHeader(64);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("Implicit header failed: ");
        Serial.println(state);
        while (true);
    }

    lastPacketTime = millis();

    Serial.println("LoRa initialized!");
    Serial.println("Waiting for packets...");

    updateDisplay();
}

void loop() {
    int state = radio.receive(packet, sizeof(packet), 10000);

    if (state == RADIOLIB_ERR_NONE) {
        unsigned long packetNumber = 0;

        sscanf((char*)packet, "PACKET:%lu", &packetNumber);

        if (!firstPacket && packetNumber > lastPacketNumber + 1) {
            lostPackets += packetNumber - lastPacketNumber - 1;
        }

        firstPacket = false;
        lastPacketNumber = packetNumber;
        receivedPackets++;

        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();

        lastPacketTime = millis();
        linkLost = false;

        Serial.print("Packet: ");
        Serial.println(packetNumber);

        Serial.print("RSSI: ");
        Serial.println(lastRSSI);

        Serial.print("SNR: ");
        Serial.println(lastSNR);

        updateDisplay();
    }

    if (millis() - lastPacketTime > 15000) {
        if (!linkLost) {
            linkLost = true;

            Serial.println("LINK LOST");

            updateDisplay();
        }
    }
}