#include <Arduino.h>

const int BUTTON_PIN = 32;
const int MOSFET_PIN = 25;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MOSFET_PIN, OUTPUT);
}

void loop() {
  int val = digitalRead(BUTTON_PIN);
  if (!val) {
     digitalWrite(MOSFET_PIN, HIGH);
     Serial.println(String("МОСФЕТ ПРАЦЮЙ! ") + val);
  } else {
     digitalWrite(MOSFET_PIN, LOW);
     Serial.println(String("МОСФЕТ ВИМКНУТИЙ: ") + val);
  }

  delay(200);
}