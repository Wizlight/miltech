#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// BUTTONS
// ======================================================

const int BUTTON_MODE_PIN = 4;
const int BUTTON_SYNC_PIN = 13;

// ======================================================
// OLED
// ======================================================

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

const int MY_OLED_SDA = 21;
const int MY_OLED_SCL = 22;

const int OLED_RESET = -1;
const int OLED_ADDRESS = 0x3C;

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ======================================================
// PROFILES
// ======================================================

enum RadioProfile {
  NORMAL,
  FAST,
  LONG_RANGE
};

RadioProfile selectedProfile = NORMAL;

// ======================================================
// BUTTON STATE
// ======================================================

bool lastModeState = HIGH;
bool lastSyncState = HIGH;

unsigned long lastModePress = 0;
unsigned long lastSyncPress = 0;

const unsigned long DEBOUNCE_MS = 200;

// ======================================================
// HELPERS
// ======================================================

const char* getProfileName() {
  switch (selectedProfile) {
    case NORMAL:
      return "NORMAL";

    case FAST:
      return "FAST";

    case LONG_RANGE:
      return "LONG";
  }

  return "?";
}

void showScreen(const char* action) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("BUTTON TEST");
  display.println();

  display.print("MODE: GPIO");
  display.println(BUTTON_MODE_PIN);

  display.print("SYNC: GPIO");
  display.println(BUTTON_SYNC_PIN);

  display.println();

  display.print("Profile: ");
  display.println(getProfileName());

  display.print("Action: ");
  display.println(action);

  display.display();
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SYNC_PIN, INPUT_PULLUP);

  Wire.begin(
    MY_OLED_SDA,
    MY_OLED_SCL
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println("OLED init failed");

    while (true) {
    }
  }

  Serial.println("Button test started");
  Serial.println("MODE = GPIO4");
  Serial.println("SYNC = GPIO13");

  showScreen("READY");
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  bool modeState = digitalRead(BUTTON_MODE_PIN);
  bool syncState = digitalRead(BUTTON_SYNC_PIN);

  // MODE
  if (
    modeState == LOW &&
    lastModeState == HIGH &&
    millis() - lastModePress > DEBOUNCE_MS
  ) {
    lastModePress = millis();

    if (selectedProfile == NORMAL) {
      selectedProfile = FAST;
    }
    else if (selectedProfile == FAST) {
      selectedProfile = LONG_RANGE;
    }
    else {
      selectedProfile = NORMAL;
    }

    Serial.print("MODE -> ");
    Serial.println(getProfileName());

    showScreen("MODE");
  }

  // SYNC
  if (
    syncState == LOW &&
    lastSyncState == HIGH &&
    millis() - lastSyncPress > DEBOUNCE_MS
  ) {
    lastSyncPress = millis();

    Serial.print("SYNC -> ");
    Serial.println(getProfileName());

    showScreen("SYNC");
  }

  lastModeState = modeState;
  lastSyncState = syncState;
}