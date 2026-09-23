#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>

Preferences preferences;

const int CONFIG_VERSION = 1;
const unsigned long DEFAULT_ARM_DELAY = 300000;
const int BUTTON_PIN = 32;
const int LED_PIN = 23;

const int PWM_OUT_PIN = 25;
const int PWM_IN_PIN = 34;

const int PWM_CHANNEL = 0;
const int PWM_FREQUENCY = 50;
const int PWM_RESOLUTION = 16;

const char* WIFI_SSID = "esp32";
const char* WIFI_PASSWORD = "12345678";

IPAddress apIP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

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
    INIT_WEB,
    INIT_DONE,
    INIT_FAILED
};

InitState initState = INIT_START;

enum PwmCommand {
    PWM_NONE,
    PWM_STOP,
    PWM_START,
    PWM_TRIGGER
};

PwmCommand lastPwmCommand = PWM_NONE;

unsigned long armDelayMs = 0;
unsigned long countdownStartedAt = 0;

String getStateName() {
    switch (state) {
        case SAFE: return "SAFE";
        case COUNTDOWN: return "COUNTDOWN";
        case ARMED: return "ARMED";
        case TRIGGERED: return "TRIGGERED";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void enterFailSafe(const char* reason) {
    if (state == ERROR) {
        return;
    }

    Serial.print("FAIL-SAFE: ");
    Serial.println(reason);

    event = EVENT_NONE;
    state = ERROR;
}

bool startWebServer() {
    if (!WiFi.softAPConfig(apIP, gateway, subnet)) {
        return false;
    }

    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
        return false;
    }

    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", []() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Safe Simulation Control</title>
  <style>
    * {
      box-sizing: border-box;
      font-family: Arial, sans-serif;
    }

    body {
      margin: 0;
      padding: 20px;
      background: linear-gradient(135deg, #0f172a, #1e293b);
      color: #f1f5f9;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .card {
      width: 100%;
      max-width: 420px;
      background: rgba(255,255,255,0.08);
      border: 1px solid rgba(255,255,255,0.12);
      border-radius: 20px;
      padding: 24px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.35);
      backdrop-filter: blur(10px);
    }

    h1 {
      margin-top: 0;
      margin-bottom: 8px;
      font-size: 24px;
      text-align: center;
    }

    .subtitle {
      text-align: center;
      color: #cbd5e1;
      margin-bottom: 24px;
      font-size: 14px;
    }

    .status-box {
      background: rgba(255,255,255,0.06);
      border-radius: 16px;
      padding: 18px;
      text-align: center;
      margin-bottom: 20px;
    }

    .status-label {
      font-size: 13px;
      color: #cbd5e1;
      margin-bottom: 8px;
    }

    .status-value {
      font-size: 28px;
      font-weight: bold;
      letter-spacing: 1px;
    }

    .status-safe { color: #22c55e; }
    .status-countdown { color: #f59e0b; }
    .status-armed { color: #ef4444; }
    .status-triggered { color: #e11d48; }
    .status-error { color: #a855f7; }

    .buttons {
      display: grid;
      gap: 12px;
    }

    button {
      border: none;
      border-radius: 14px;
      padding: 16px;
      font-size: 16px;
      font-weight: bold;
      cursor: pointer;
      transition: transform 0.1s ease, opacity 0.2s ease;
    }

    button:active {
      transform: scale(0.98);
    }

    .btn-start {
      background: #22c55e;
      color: white;
    }

    .btn-stop {
      background: #f59e0b;
      color: white;
    }

    .btn-trigger {
      background: #ef4444;
      color: white;
    }

    .footer {
      margin-top: 18px;
      text-align: center;
      font-size: 12px;
      color: #94a3b8;
    }

    .hint {
      margin-top: 16px;
      padding: 12px;
      background: rgba(255,255,255,0.05);
      border-radius: 12px;
      font-size: 13px;
      color: #cbd5e1;
      text-align: center;
    }
  </style>
</head>
<body>
  <div class="card">
    <h1>Safe Simulation</h1>
    <div class="subtitle">ESP32 control panel</div>

    <div class="status-box">
      <div class="status-label">Поточний стан</div>
      <div id="state" class="status-value">LOADING...</div>
    </div>

    <div class="buttons">
      <button class="btn-start" onclick="sendCommand('/start')">START</button>
      <button class="btn-stop" onclick="sendCommand('/stop')">STOP</button>
      <button class="btn-trigger" onclick="sendCommand('/trigger')">ПІДРИВ / СПРАЦЮВАННЯ</button>
    </div>

    <div class="hint">
      Після START запускається таймер безпеки.  
      У стані ARMED можна виконати trigger.
    </div>

    <div class="footer">
      Auto refresh: 1s
    </div>
  </div>

  <script>
    function applyStateStyle(state) {
      const el = document.getElementById('state');
      el.className = 'status-value';

      if (state === 'SAFE') el.classList.add('status-safe');
      else if (state === 'COUNTDOWN') el.classList.add('status-countdown');
      else if (state === 'ARMED') el.classList.add('status-armed');
      else if (state === 'TRIGGERED') el.classList.add('status-triggered');
      else if (state === 'ERROR') el.classList.add('status-error');
    }

    function updateStatus() {
      fetch('/status')
        .then(response => response.json())
        .then(data => {
          const state = data.state || 'UNKNOWN';
          document.getElementById('state').textContent = state;
          applyStateStyle(state);
        })
        .catch(() => {
          document.getElementById('state').textContent = 'OFFLINE';
        });
    }

    function sendCommand(url) {
      fetch(url, { method: 'POST' })
        .then(() => setTimeout(updateStatus, 200));
    }

    updateStatus();
    setInterval(updateStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

        server.send(200, "text/html", html);
    });

    server.on("/status", []() {
        String json = "{\"state\":\"" + getStateName() + "\"}";
        server.send(200, "application/json", json);
    });

    server.on("/start", HTTP_POST, []() {
        event = EVENT_START;
        server.send(200, "text/plain", "OK");
    });

    server.on("/stop", HTTP_POST, []() {
        event = EVENT_STOP;
        server.send(200, "text/plain", "OK");
    });

    server.on("/trigger", HTTP_POST, []() {
        event = EVENT_TRIGGER;
        server.send(200, "text/plain", "OK");
    });

    server.begin();

    Serial.println("Web server started");
    Serial.print("Open: http://");
    Serial.println(WiFi.softAPIP());

    return true;
}

struct SensorData {
    float voltage;
    bool sensorOk;
};

SensorData readSensors() {
    SensorData data;

    // Поки симуляція
    data.voltage = 5.0;
    data.sensorOk = true;

    return data;
}

void setTestPwm(int pulseWidthUs) {
    const int periodUs = 20000; // 50 Hz = 20 ms

    uint32_t duty = ((uint32_t)pulseWidthUs * 65535) / periodUs;

    ledcWrite(PWM_CHANNEL, duty);
}

bool validateSensors(const SensorData& data) {
    if (!data.sensorOk) {
        return false;
    }

    if (data.voltage < 3.0 || data.voltage > 5.5) {
        return false;
    }

    return true;
}

void checkSensors() {
    SensorData data = readSensors();

    if (!validateSensors(data)) {
        enterFailSafe("Invalid sensor data");
    }
}

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
    uint64_t deviceId = ESP.getEfuseMac();

    Serial.print("Device ID: ");
    Serial.printf("%04X%08X\n",
        (uint16_t)(deviceId >> 32),
        (uint32_t)deviceId
    );

    return deviceId != 0;
}

bool checkConfig() {
    preferences.begin("safe_sim", false);

    if (!preferences.isKey("version")) {
        preferences.putInt("version", CONFIG_VERSION);
        preferences.putULong("arm_delay", DEFAULT_ARM_DELAY);
    }

    int version = preferences.getInt("version", -1);
    unsigned long armDelay = preferences.getULong("arm_delay", 0);

    preferences.end();

    if (version != CONFIG_VERSION) {
        return false;
    }

    if (armDelay == 0) {
        return false;
    }

    armDelayMs = armDelay;

    return true;
}

bool checkButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    int buttonState = digitalRead(BUTTON_PIN);

    return buttonState == HIGH;
}

bool checkComponents() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

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
                Serial.println("INIT: filesystem FAILED");
                initState = INIT_FAILED;
            }
            break;

        case INIT_DEVICE_ID:
            if (checkDeviceId()) {
                Serial.println("INIT: device ID OK");
                initState = INIT_CONFIG;
            } else {
                Serial.println("INIT: device ID FAILED");
                initState = INIT_FAILED;
            }
            break;

        case INIT_CONFIG:
            if (checkConfig()) {
                Serial.println("INIT: config OK");
                initState = INIT_BUTTON;
            } else {
                Serial.println("INIT: config FAILED");
                initState = INIT_FAILED;
            }
            break;

        case INIT_BUTTON:
            if (checkButton()) {
                Serial.println("INIT: button OK");
                initState = INIT_COMPONENTS;
            } else {
                Serial.println("INIT: button FAILED");
                initState = INIT_FAILED;
            }
            break;

       case INIT_COMPONENTS:
            if (checkComponents()) {
                Serial.println("INIT: components OK");
                initState = INIT_WEB;
            } else {
                Serial.println("INIT: components FAILED");
                initState = INIT_FAILED;
            }
            break;
        case INIT_WEB:
            if (startWebServer()) {
                Serial.println("INIT: web server OK");
                initState = INIT_DONE;
            } else {
                Serial.println("INIT: web server FAILED");
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
            else if (millis() - countdownStartedAt >= armDelayMs) {
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
            // Нічого не робимо.
            // Система залишається заблокованою до перезапуску.
            break;
    }

    event = EVENT_NONE;
}

void readPwmCommand() {
    unsigned long pulse = pulseIn(PWM_IN_PIN, HIGH, 25000);

    if (pulse == 0) {
        return;
    }

    PwmCommand command = PWM_NONE;

    if (pulse >= 900 && pulse <= 1100) {
        command = PWM_STOP;
    }
    else if (pulse >= 1400 && pulse <= 1600) {
        command = PWM_START;
    }
    else if (pulse >= 1900 && pulse <= 2100) {
        command = PWM_TRIGGER;
    }
    else {
        enterFailSafe("Invalid PWM command");
        return;
    }

    if (command == lastPwmCommand) {
        return;
    }

    lastPwmCommand = command;

    if (command == PWM_STOP) {
        Serial.println("PWM received: STOP");
        event = EVENT_STOP;
    }
    else if (command == PWM_START) {
        Serial.println("PWM received: START");
        event = EVENT_START;
    }
    else if (command == PWM_TRIGGER) {
        Serial.println("PWM received: TRIGGER");
        event = EVENT_TRIGGER;
    }
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
    else if (command == "pwm stop") {
        setTestPwm(1000);
    }
    else if (command == "pwm start") {
        setTestPwm(1500);
    }
    else if (command == "pwm trigger") {
        setTestPwm(2000);
    }
}

void updateLed() {
    switch (state) {
        case SAFE:
            digitalWrite(LED_PIN, LOW);
            break;

        case COUNTDOWN:
            digitalWrite(LED_PIN, (millis() / 500) % 2);
            break;

        case ARMED:
            digitalWrite(LED_PIN, HIGH);
            break;

        case TRIGGERED:
            digitalWrite(LED_PIN, (millis() / 100) % 2);
            break;

        case ERROR:
            digitalWrite(LED_PIN, LOW);
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("System started");

    pinMode(PWM_IN_PIN, INPUT);

    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(PWM_OUT_PIN, PWM_CHANNEL);

    setTestPwm(1000);
    lastPwmCommand = PWM_STOP;
}

void loop() {
    if (initState != INIT_DONE) {
        updateInitStateMachine();
        return;
    }

    server.handleClient();

    checkSensors();

    if (state == ERROR) {
        updateLed();
        return;
    }

    readCommand();
    readPwmCommand();

    updateStateMachine();
    updateLed();
}