#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.print("Build hash: ");
    Serial.println(BUILD_HASH);

    Serial.print("Dirty: ");
    Serial.println(BUILD_DIRTY ? "yes" : "no");
}

void loop() {
}