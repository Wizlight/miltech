#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================================
// LORA
// ======================================================

const int LORA_SCK  = 5;
const int LORA_MISO = 19;
const int LORA_MOSI = 27;
const int LORA_CS   = 18;
const int LORA_RST  = 23;
const int LORA_DIO0 = 26;

SX1276 radio = new Module(
  LORA_CS,
  LORA_DIO0,
  LORA_RST
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


// ======================================================
// OLED
// ======================================================

void showStatus(const String& status) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("LoRa TX");

  display.println();

  display.print("SF: ");
  display.print(SPREADING_FACTOR);

  display.print("  BW: ");
  display.println((int)BANDWIDTH);

  display.print("Power: ");
  display.print(TX_POWER);
  display.println(" dBm");

  display.print("Packet: ");
  display.println(packetNumber);

  display.println();

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

  // ---------- OLED ----------

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


  // ---------- SPI ----------

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_CS
  );


  // ---------- RADIO ----------

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

    showStatus("READY");
  } else {
    Serial.print("Radio init FAIL: ");
    Serial.println(state);

    showStatus("RADIO ERROR");

    while (true) {
    }
  }
}


// ======================================================
// LOOP
// ======================================================

void loop() {
  String message =
    "ping " + String(packetNumber);

  Serial.print("Sending: ");
  Serial.println(message);

  int state = radio.transmit(message);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("TX OK");

    showStatus("SENT");
  } else {
    Serial.print("TX ERROR: ");
    Serial.println(state);

    showStatus("ERROR");
  }

  packetNumber++;

  delay(2000);
}