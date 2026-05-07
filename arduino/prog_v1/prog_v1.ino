

//  ESP32dfmini01
//
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "Arduino.h"

const byte RXD2 = 7;   // ESP32 RX  -> TX du module
const byte TXD2 = 6;   // ESP32 TX  -> RX du module

HardwareSerial dfSD(1); // UART1
DFRobotDFPlayerMini player;

void setup() {
  Serial.begin(115200);          // moniteur série plus rapide
  delay(2000);

  dfSD.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(3000);                   // laisser le temps au DFPlayer de booter

  Serial.println("Init DFPlayer...");
  if (player.begin(dfSD)) {
    Serial.println("DFPlayer OK");
    player.volume(17);           // 0..30
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