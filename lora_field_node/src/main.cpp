#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <SPI.h>
#include <RadioLib.h>

SX1276 radio = new Module(18, 26, 23, RADIOLIB_NC);

uint8_t postResult = 0;

const uint8_t POST_FLASH_FAIL = 1 << 0;
const uint8_t POST_NVS_FAIL   = 1 << 1;
const uint8_t POST_OLED_FAIL  = 1 << 2;
const uint8_t POST_RADIO_FAIL = 1 << 3;

const uint8_t PROTOCOL_VERSION = 1;
const uint8_t TELEMETRY_POST = 0x01;

uint8_t telemetrySequence = 0;

#define FW_VERSION "0.0.1"

String commandBuffer = "";

Preferences preferences;

const int CURRENT_CFG_VERSION = 2;

const int DEFAULT_TX_POWER = 10;
const int DEFAULT_SEND_INTERVAL = 1000;
const int DEFAULT_MAX_RETRIES = 3;

struct Config {
    int cfgVersion;
    int txPower;
    int sendInterval;
    int maxRetries;
};

Config config;

enum LogLevel {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

const int LOG_SIZE = 10;

String logBuffer[LOG_SIZE];

int logIndex = 0;
int logCount = 0;

void logMessage(LogLevel level, String message) {
    String line;

    if (level == LOG_INFO) {
        line = "[INFO] ";
    }

    if (level == LOG_WARN) {
        line = "[WARN] ";
    }

    if (level == LOG_ERROR) {
        line = "[ERROR] ";
    }

    line += message;

    Serial.println(line);

    logBuffer[logIndex] = line;

    logIndex++;

    if (logIndex >= LOG_SIZE) {
        logIndex = 0;
    }

    if (logCount < LOG_SIZE) {
        logCount++;
    }
}

bool testFlash() {
    return ESP.getFlashChipSize() >= 4 * 1024 * 1024;
}

bool testNVS() {
    preferences.begin("post", false);

    uint32_t testValue = 123456;
    preferences.putUInt("test", testValue);

    uint32_t readValue = preferences.getUInt("test", 0);

    preferences.remove("test");
    preferences.end();

    return readValue == testValue;
}

bool testOLED() {
    Wire.begin(21, 22);

    Wire.beginTransmission(0x3C);
    int result = Wire.endTransmission();

    return result == 0;
}

bool testRadio() {
    SPI.begin(5, 19, 27, 18);

    int state = radio.begin(867.5);

    return state == RADIOLIB_ERR_NONE;
}

void printLog() {
    int index = logIndex - logCount;

    if (index < 0) {
        index += LOG_SIZE;
    }

    for (int i = 0; i < logCount; i++) {
        Serial.println(logBuffer[index]);

        index++;

        if (index == LOG_SIZE) {
            index = 0;
        }
    }
}

void printConfig() {
    Serial.print("cfgVersion: ");
    Serial.println(config.cfgVersion);

    Serial.print("txPower: ");
    Serial.println(config.txPower);

    Serial.print("sendInterval: ");
    Serial.println(config.sendInterval);

    Serial.print("maxRetries: ");
    Serial.println(config.maxRetries);
}

void setDefaultConfig() {
    config.cfgVersion = CURRENT_CFG_VERSION;
    config.txPower = DEFAULT_TX_POWER;
    config.sendInterval = DEFAULT_SEND_INTERVAL;
    config.maxRetries = DEFAULT_MAX_RETRIES;
}

void saveConfig() {
    preferences.begin("config", false);

    preferences.putInt("cfg_version", config.cfgVersion);
    preferences.putInt("tx_power", config.txPower);
    preferences.putInt("send_interval", config.sendInterval);
    preferences.putInt("max_retries", config.maxRetries);

    preferences.end();
}

void resetConfig() {
    setDefaultConfig();
    saveConfig();

    logMessage(LOG_INFO, "Config reset to defaults");
}

void loadConfig() {
    preferences.begin("config", false);

    bool configExists = preferences.isKey("cfg_version");

    if (!configExists) {
        preferences.end();

        setDefaultConfig();
        saveConfig();

        logMessage(LOG_INFO, "Default config saved to NVS");
        return;
    }

    config.cfgVersion =
        preferences.getInt("cfg_version", 1);

    config.txPower =
        preferences.getInt("tx_power", DEFAULT_TX_POWER);

    config.sendInterval =
        preferences.getInt("send_interval", DEFAULT_SEND_INTERVAL);


    // Migration: version 1 -> version 2
    if (config.cfgVersion == 1) {
        preferences.end();

        config.maxRetries = DEFAULT_MAX_RETRIES;
        config.cfgVersion = 2;

        saveConfig();

        logMessage(LOG_INFO, "Config migrated from v1 to v2");
        return;
    }


    config.maxRetries =
        preferences.getInt("max_retries", DEFAULT_MAX_RETRIES);

    preferences.end();

    logMessage(LOG_INFO, "Config loaded from NVS");
}

void setConfig(String key, int value) {
    if (key == "txPower") {
        if (value < 2 || value > 20) {
            logMessage(LOG_WARN, "txPower must be 2..20");
            return;
        }

        config.txPower = value;
        saveConfig();

        logMessage(LOG_INFO, "txPower changed");
        return;
    }

    if (key == "sendInterval") {
        if (value < 100 || value > 60000) {
            logMessage(LOG_WARN, "sendInterval must be 100..60000");
            return;
        }

        config.sendInterval = value;
        saveConfig();

        logMessage(LOG_INFO, "sendInterval changed");
        return;
    }

    if (key == "maxRetries") {
        if (value < 0 || value > 10) {
            logMessage(LOG_WARN, "maxRetries must be 0..10");
            return;
        }

        config.maxRetries = value;
        saveConfig();

        logMessage(LOG_INFO, "maxRetries changed to " + String(value));
        return;
    }

    logMessage(LOG_WARN, "Unknown config key: " + key);
}

void handleCommand(String command) {
    command.trim();

    if (command == "version") {
        logMessage(LOG_INFO, "Version command");

        Serial.print("Version: ");
        Serial.println(FW_VERSION);

        Serial.print("Build hash: ");
        Serial.println(BUILD_HASH);

        Serial.print("Dirty: ");
        Serial.println(BUILD_DIRTY ? "yes" : "no");
    }
    else if (command == "log") {
        printLog();
    }
    else if (command == "config get") {
        logMessage(LOG_INFO, "Config get");
        printConfig();
    }
    else if (command.startsWith("config set ")) {
        String parameters = command.substring(11);

        int spacePosition = parameters.indexOf(' ');

        if (spacePosition == -1) {
            logMessage(LOG_WARN, "Invalid config set command");
            return;
        }

        String key = parameters.substring(0, spacePosition);
        String valueText = parameters.substring(spacePosition + 1);

        int value = valueText.toInt();

        setConfig(key, value);
    }
    else if (command == "config reset") {
        resetConfig();
    }
    else {
        logMessage(LOG_WARN, "Unknown command: " + command);
    }
}

uint8_t runPOST() {
    uint8_t result = 0;

    if (testFlash()) {
        logMessage(LOG_INFO, "POST Flash: OK");
    } else {
        result |= POST_FLASH_FAIL;
        logMessage(LOG_ERROR, "POST Flash: FAIL");
    }

    if (testNVS()) {
        logMessage(LOG_INFO, "POST NVS: OK");
    } else {
        result |= POST_NVS_FAIL;
        logMessage(LOG_ERROR, "POST NVS: FAIL");
    }

    if (testOLED()) {
        logMessage(LOG_INFO, "POST OLED: OK");
    } else {
        result |= POST_OLED_FAIL;
        logMessage(LOG_ERROR, "POST OLED: FAIL");
    }

    if (testRadio()) {
        logMessage(LOG_INFO, "POST Radio: OK");
    } else {
        result |= POST_RADIO_FAIL;
        logMessage(LOG_ERROR, "POST Radio: FAIL");
    }

    logMessage(LOG_INFO, "POST mask: 0x" + String(result, HEX));

    return result;
}

void sendPostTelemetry(uint8_t postMask) {
    uint8_t packet[5];

    packet[0] = PROTOCOL_VERSION;
    packet[1] = TELEMETRY_POST;
    packet[2] = telemetrySequence;
    packet[3] = 1;
    packet[4] = postMask;

    int state = radio.transmit(packet, sizeof(packet));

    if (state == RADIOLIB_ERR_NONE) {
        logMessage(LOG_INFO, "POST telemetry sent");
        telemetrySequence++;
    } else {
        logMessage(LOG_ERROR, "POST telemetry send failed");
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    logMessage(LOG_INFO, "Device started");
    loadConfig();
    postResult = runPOST();

    if ((postResult & POST_RADIO_FAIL) == 0) {
        sendPostTelemetry(postResult);
    } else {
        logMessage(LOG_ERROR, "POST telemetry not sent: radio failed");
    }
}

void loop() {
    while (Serial.available()) {
        char c = Serial.read();
        // Enter
        if (c == '\n' || c == '\r') {
            if (commandBuffer.length() > 0) {
                Serial.println();
                handleCommand(commandBuffer);
                commandBuffer = "";
            }
        }

        // Backspace або Delete
        else if (c == '\b' || c == 127) {
            if (commandBuffer.length() > 0) {
                commandBuffer.remove(commandBuffer.length() - 1);

                Serial.print("\b \b");
            }
        }

        // Звичайний символ
        else if (c >= 32 && c <= 126) {
            commandBuffer += c;
            Serial.print(c);
        }
    }
}