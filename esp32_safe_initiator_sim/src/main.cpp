#include <Arduino.h>
#include <LittleFS.h>

enum SystemState {
    SAFE,
    COUNTDOWN,
    ARMED,
    TRIGGERED,
    ERROR
};

SystemState state = SAFE;

enum SystemEvent {
    EVENT_NONE,
    EVENT_START,
    EVENT_STOP,
    EVENT_TRIGGER
};

SystemEvent event = EVENT_NONE;

enum InitState {
    INIT_START,
    INIT_FILESYSTEM,
    INIT_DEVICE_ID,
    INIT_CONFIG,
    INIT_BUTTON,
    INIT_COMPONENTS,
    INIT_DONE,
    INIT_FAILED
};

InitState initState = INIT_START;

const unsigned long ARM_DELAY = 10000;
unsigned long countdownStartedAt = 0;

bool checkFilesystem() {
    if (!LittleFS.begin(false)) {
        return false;
    }

    File file = LittleFS.open("/selftest.txt", "w");

    if (!file) {
        return false;
    }

    file.print("OK");
    file.close();

    file = LittleFS.open("/selftest.txt", "r");

    if (!file) {
        return false;
    }

    String value = file.readString();
    file.close();

    LittleFS.remove("/selftest.txt");

    return value == "OK";
}

bool checkDeviceId() {
    return true;
}

bool checkConfig() {
    return true;
}

bool checkButton() {
    return true;
}

bool checkComponents() {
    return true;
}

void updateInitStateMachine() {
    switch (initState) {
        case INIT_START:
            Serial.println("INIT: start");
            initState = INIT_FILESYSTEM;
            break;

        case INIT_FILESYSTEM:
            if (checkFilesystem()) {
                Serial.println("INIT: filesystem OK");
                initState = INIT_DEVICE_ID;
            } else {
                initState = INIT_FAILED;
            }
            break;

        case INIT_DEVICE_ID:
            if (checkDeviceId()) {
                Serial.println("INIT: device ID OK");
                initState = INIT_CONFIG;
            } else {
                initState = INIT_FAILED;
            }
            break;

        case INIT_CONFIG:
            if (checkConfig()) {
                Serial.println("INIT: config OK");
                initState = INIT_BUTTON;
            } else {
                initState = INIT_FAILED;
            }
            break;

        case INIT_BUTTON:
            if (checkButton()) {
                Serial.println("INIT: button OK");
                initState = INIT_COMPONENTS;
            } else {
                initState = INIT_FAILED;
            }
            break;

        case INIT_COMPONENTS:
            if (checkComponents()) {
                Serial.println("INIT: components OK");
                initState = INIT_DONE;
            } else {
                initState = INIT_FAILED;
            }
            break;

        case INIT_DONE:
            break;

       case INIT_FAILED:
            break;
    }
}

void updateStateMachine() {
    switch (state) {
        case SAFE:
            if (event == EVENT_START) {
                state = COUNTDOWN;
                countdownStartedAt = millis();
                Serial.println("SAFE -> COUNTDOWN");
            }
            break;

        case COUNTDOWN:
            if (event == EVENT_STOP) {
                state = SAFE;
                Serial.println("COUNTDOWN -> SAFE");
            }
            else if (millis() - countdownStartedAt >= ARM_DELAY) {
                state = ARMED;
                Serial.println("COUNTDOWN -> ARMED");
            }
            break;

        case ARMED:
            if (event == EVENT_STOP) {
                state = SAFE;
                Serial.println("ARMED -> SAFE");
            }
            else if (event == EVENT_TRIGGER) {
                state = TRIGGERED;
                Serial.println("ARMED -> TRIGGERED");
            }
            break;

        case TRIGGERED:
            if (event == EVENT_STOP) {
                state = SAFE;
                Serial.println("TRIGGERED -> SAFE");
            }
            break;

        case ERROR:
            state = SAFE;
            Serial.println("ERROR -> SAFE");
            break;
    }

    event = EVENT_NONE;
}

void readCommand() {
    if (!Serial.available()) {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "start") {
        event = EVENT_START;
    }
    else if (command == "stop") {
        event = EVENT_STOP;
    }
    else if (command == "trigger") {
        event = EVENT_TRIGGER;
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println("System started");
}

void loop() {
    if (initState != INIT_DONE) {
        updateInitStateMachine();
        return;
    }

    readCommand();
    updateStateMachine();
}