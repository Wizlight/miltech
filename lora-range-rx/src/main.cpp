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
// STATISTICS
// ======================================================

unsigned long receivedCount = 0;
unsigned long lostCount = 0;

long lastPacketNumber = -1;

float lastRSSI = 0;
float lastSNR = 0;


// ======================================================
// OLED
// ======================================================

void showWaiting() {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("LoRa RX");

  display.println();

  display.print("SF: ");
  display.print(SPREADING_FACTOR);

  display.print("  BW: ");
  display.println((int)BANDWIDTH);

  display.println();
  display.println("Waiting...");

  display.display();
}


void showPacket(const String& message) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("LoRa RX");

  display.print("Packet: ");
  display.println(message);

  display.print("RSSI: ");
  display.print(lastRSSI, 1);
  display.println(" dBm");

  display.print("SNR: ");
  display.print(lastSNR, 1);
  display.println(" dB");

  display.print("RX: ");
  display.print(receivedCount);

  display.print(" Lost: ");
  display.println(lostCount);

  display.display();
}


// ======================================================
// EXTRACT PACKET NUMBER
// ======================================================

long getPacketNumber(const String& message) {
  if (!message.startsWith("ping ")) {
    return -1;
  }

  String number =
    message.substring(5);

  return number.toInt();
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

    showWaiting();
  } else {
    Serial.print("Radio init FAIL: ");
    Serial.println(state);

    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);

    display.println("RADIO ERROR");
    display.println(state);

    display.display();

    while (true) {
    }
  }
}


// ======================================================
// LOOP
// ======================================================

void loop() {
  String message;

  int state = radio.receive(message);

  if (state == RADIOLIB_ERR_NONE) {

    receivedCount++;

    lastRSSI = radio.getRSSI();
    lastSNR = radio.getSNR();


    // ---------- PACKET LOSS ----------

    long packetNumber =
      getPacketNumber(message);

    if (packetNumber >= 0) {

      if (
        lastPacketNumber >= 0 &&
        packetNumber > lastPacketNumber + 1
      ) {

        lostCount +=
          packetNumber - lastPacketNumber - 1;
      }

      lastPacketNumber = packetNumber;
    }


    // ---------- SERIAL ----------

    Serial.print(message);

    Serial.print(" | RSSI ");
    Serial.print(lastRSSI);

    Serial.print(" dBm | SNR ");
    Serial.print(lastSNR);

    Serial.print(" dB | RX ");
    Serial.print(receivedCount);

    Serial.print(" | Lost ");
    Serial.println(lostCount);


    // ---------- OLED ----------

    showPacket(message);
  }

  else if (state == RADIOLIB_ERR_CRC_MISMATCH) {

    Serial.println("CRC error");
  }

  else {

    Serial.print("RX error: ");
    Serial.println(state);
  }
}