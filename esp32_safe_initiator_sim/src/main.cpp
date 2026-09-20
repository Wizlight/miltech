#include <Arduino.h>

enum SystemState {
    SAFE,
    COUNTDOWN,
    ARMED,
    TRIGGERED,
    ERROR
};

SystemState state = SAFE;

void updateStateMachine() {
    switch (state) {
        case SAFE:
            break;

        case COUNTDOWN:
            break;

        case ARMED:
            break;

        case TRIGGERED:
            break;

        case ERROR:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("System started");
}

void loop() {
    updateStateMachine();
}