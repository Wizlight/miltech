#include <Arduino.h>
#include <RadioLib.h>
#include <U8g2lib.h>
#include <Wire.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE
);

void setup() {
    Wire.begin(21, 22);
    display.begin();

    SPI.begin(5, 19, 27, 18);
    radio.begin(867.5);
}

void loop() {
    String message;

    if (radio.receive(message) == RADIOLIB_ERR_NONE) {
        display.clearBuffer();
        display.setFont(u8g2_font_6x12_tf);
        display.drawStr(0, 32, message.c_str());
        display.sendBuffer();
    }
}