#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// LORA PINS
// ======================================================

const int RADIO_SCK_PIN  = 5;
const int RADIO_MISO_PIN = 19;
const int RADIO_MOSI_PIN = 27;
const int RADIO_CS_PIN   = 18;
const int RADIO_RST_PIN  = 23;
const int RADIO_DIO0_PIN = 26;

SX1276 radio = new Module(
  RADIO_CS_PIN,
  RADIO_DIO0_PIN,
  RADIO_RST_PIN
);

// ======================================================
// RADIO CONFIG
// ======================================================

const float FREQUENCY = 915.0;
const float BANDWIDTH = 125.0;

const int SPREADING_FACTOR = 9;
const int CODING_RATE = 7;
const int TX_POWER = 17;

const uint8_t SYNC_WORD = 0x12;

// 15 секунд між передачами
const unsigned long SEND_INTERVAL = 15000;

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
// STATE
// ======================================================

unsigned long packetNumber = 0;
unsigned long lastSentTime = 0;

// ======================================================
// TIME FORMAT
// ======================================================

String formatTime(unsigned long milliseconds) {
  unsigned long totalSeconds = milliseconds / 1000;

  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;
  int seconds = totalSeconds % 60;

  char buffer[9];

  sprintf(
    buffer,
    "%02d:%02d:%02d",
    hours,
    minutes,
    seconds
  );

  return String(buffer);
}

// ======================================================
// OLED
// ======================================================

void showStatus(
  const String& status,
  unsigned long sentPacket
) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("LoRa TX");

  display.print("SF");
  display.print(SPREADING_FACTOR);

  display.print(" BW");
  display.println((int)BANDWIDTH);

  display.print("Power: ");
  display.print(TX_POWER);
  display.println(" dBm");

  display.print("Packet: ");
  display.println(sentPacket);

  display.print("Last: ");
  display.println(formatTime(lastSentTime));

  display.print("Status: ");
  display.println(status);

  display.display();
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);

  delay(1000);

  // OLED
  Wire.begin(MY_OLED_SDA, MY_OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println("OLED init failed");

    while (true) {
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("Starting TX...");
  display.display();

  // SPI
  SPI.begin(
    RADIO_SCK_PIN,
    RADIO_MISO_PIN,
    RADIO_MOSI_PIN,
    RADIO_CS_PIN
  );

  // LoRa
  int state = radio.begin(
    FREQUENCY,
    BANDWIDTH,
    SPREADING_FACTOR,
    CODING_RATE,
    SYNC_WORD,
    TX_POWER
  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio init OK");
  } else {
    Serial.print("Radio init FAIL: ");
    Serial.println(state);

    while (true) {
    }
  }
}

// ======================================================
// LOOP
// ======================================================

void loop() {
  String message = "ping " + String(packetNumber);

  Serial.print("Sending: ");
  Serial.println(message);

  int state = radio.transmit(message);

  if (state == RADIOLIB_ERR_NONE) {

    // Запам'ятовуємо момент успішної передачі
    lastSentTime = millis();

    Serial.print("TX OK at ");
    Serial.println(formatTime(lastSentTime));

    showStatus(
      "SENT",
      packetNumber
    );

  } else {

    Serial.print("TX ERROR: ");
    Serial.println(state);

    showStatus(
      "ERROR",
      packetNumber
    );
  }

  packetNumber++;

  delay(SEND_INTERVAL);
}