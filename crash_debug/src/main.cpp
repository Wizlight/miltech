#include <Arduino.h>

void crashHere() {
    int* ptr = nullptr;
    *ptr = 123;
}

void level2() {
    crashHere();
}

void level1() {
    level2();
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("About to crash...");

    level1();
}

void loop() {
}