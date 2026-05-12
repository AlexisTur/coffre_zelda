#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "Arduino.h"

// GPIO sûrs sur ESP32-S3 SuperMini
const byte RXD2 = 7;   // ESP32 RX ← TX du DFPlayer
const byte TXD2 = 6;   // ESP32 TX → RX du DFPlayer

HardwareSerial dfSD(1);  // UART1 (pas 3 !)
DFRobotDFPlayerMini player;

void setup() {
  pinMode(12,OUTPUT);
  digitalWrite(12,1);
  Serial.begin(9600);
  delay(2000);

  dfSD.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(3000);

  Serial.println("Init DFPlayer...");
  //delay(5000);
  if (player.begin(dfSD)) {
    Serial.println("DFPlayer OK");
    player.volume(17);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
}

void loop() {
  Serial.println("Playing #1");
  player.play(1);
  delay(6000);

  Serial.println("Playing #2");
  player.play(2);
  delay(11000);
}