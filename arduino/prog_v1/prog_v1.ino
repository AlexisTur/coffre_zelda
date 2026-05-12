#include <Arduino.h>
#include "driver/rtc_io.h"
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// =========================
// Configuration
// =========================

constexpr gpio_num_t WAKEUP_GPIO = GPIO_NUM_5;
constexpr uint8_t EN_PIN = 12;

constexpr uint8_t SERVO_PIN = 13;

constexpr uint32_t TIME_WINDOW_MS = 5000;
constexpr uint32_t DEBOUNCE_MS    = 600;
constexpr uint8_t REQUIRED_PULSES = 3;
constexpr uint32_t TIME_OPENED_MS = 10000;

const byte RXD2 = 7;
const byte TXD2 = 6;
HardwareSerial dfSD(1);
DFRobotDFPlayerMini player;

// =========================
// LEDC / LED Fade
// =========================

#define LEDC_TIMER_12_BIT  12
#define LEDC_BASE_FREQ     5000
#define LED_PIN_BLUE       10
#define LED_PIN_RED        11


#define LEDC_START_DUTY_BLUE   1000
#define LEDC_TARGET_DUTY_BLUE  4095
#define LEDC_START_DUTY_RED    330
#define LEDC_TARGET_DUTY_RED   1500
#define LEDC_FADE_TIME         2000

volatile bool fade_ended = false;
bool fade_in = true;
bool leds_running = false;

const int freq_servo       = 50;
const int resolution_servo = 14;
int val_pwm_closed = 14300;
int val_pwm_opened = 15100;


void ARDUINO_ISR_ATTR LED_FADE_ISR() {
  fade_ended = true;
}

// =========================
// Variables
// =========================

uint8_t pulseCount = 0;
unsigned long startTime     = 0;
unsigned long lastPulseTime = 0;

enum State {
  SLEEP,
  VERIF,
  INIT,
  OPENING,
  WAIT,
  CLOSING
};

State state = VERIF;

// =========================
// Fonctions
// =========================

void initMP3Player() {
  dfSD.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  Serial.println("Init DFPlayer...");
  if (player.begin(dfSD, false)) {
    Serial.println("DFPlayer OK");
    player.volume(10);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
}

void startLedFade() {
  leds_running = true;
  fade_in = true;
  fade_ended = true;  // On triche : on simule une fin de fade pour que updateLedFade() démarre tout seul

  ledcAttach(LED_PIN_BLUE, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(LED_PIN_RED,  LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
}

void stopLedFade() {
  leds_running = false;
  fade_ended = false;
  ledcWrite(LED_PIN_BLUE, 0);
  ledcWrite(LED_PIN_RED,  0);
  ledcDetach(LED_PIN_BLUE);
  ledcDetach(LED_PIN_RED);
}

void updateLedFade() {
  if (!leds_running) return;
  if (!fade_ended) return;

  fade_ended = false;

  if (fade_in) {
    ledcFadeWithInterrupt(LED_PIN_BLUE, LEDC_START_DUTY_BLUE, LEDC_TARGET_DUTY_BLUE, LEDC_FADE_TIME, LED_FADE_ISR);
    ledcFadeWithInterrupt(LED_PIN_RED,  LEDC_START_DUTY_RED,  LEDC_TARGET_DUTY_RED,  LEDC_FADE_TIME, LED_FADE_ISR);
    fade_in = false;
  } else {
    ledcFadeWithInterrupt(LED_PIN_BLUE, LEDC_TARGET_DUTY_BLUE, LEDC_START_DUTY_BLUE, LEDC_FADE_TIME, LED_FADE_ISR);
    ledcFadeWithInterrupt(LED_PIN_RED,  LEDC_TARGET_DUTY_RED,  LEDC_START_DUTY_RED,  LEDC_FADE_TIME, LED_FADE_ISR);
    fade_in = true;
  }
}

void goToSleep(const char* reason) {
  Serial.println(reason);
  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);
  delay(100);
  esp_deep_sleep_start();
}

bool pulseDetected() {
  static bool previousState = false;
  bool currentState = digitalRead(WAKEUP_GPIO);

  if (currentState && !previousState) {
    unsigned long now = millis();
    if ((now - lastPulseTime) > DEBOUNCE_MS) {
      lastPulseTime = now;
      previousState = currentState;
      return true;
    }
  }

  previousState = currentState;
  return false;
}

// =========================
// Setup / Loop
// =========================

void setup() {
  Serial.begin(115200);
  pinMode(WAKEUP_GPIO, INPUT);
  pinMode(EN_PIN, OUTPUT);

  startTime  = millis();
  pulseCount = 1;

  Serial.println("Wakeup");
}

void loop() {
  switch (state) {

    case SLEEP:
      goToSleep((pulseCount == REQUIRED_PULSES) ? "Done -> Sleep" : "Timeout -> Sleep");
      break;

    case VERIF:
    {
      unsigned long now = millis();

      if ((now - startTime) >= TIME_WINDOW_MS) {
        state = SLEEP;
      }

      if (pulseDetected()) {
        pulseCount++;
        Serial.print("Pulse count: ");
        Serial.println(pulseCount);
      }

      if (pulseCount >= REQUIRED_PULSES) {
        Serial.println(String(REQUIRED_PULSES) + " pulses detected");
        state = INIT;
      }
      break;
    }

    case INIT:
      digitalWrite(EN_PIN, HIGH);
      initMP3Player();
      startLedFade();  
      ledcAttach(SERVO_PIN, freq_servo, resolution_servo); //init pwm du servo
      state = OPENING;
      break;

    case OPENING:
      Serial.println("Playing #1");
      player.play(1);
      delay(1000);
      updateLedFade(); 
      delay(1600);
      ledcWrite(SERVO_PIN, val_pwm_opened);
      startTime = millis();
      state = WAIT;
      break;

    case WAIT:
    {
      updateLedFade(); 

      unsigned long now = millis();
      if ((now - startTime) >= TIME_OPENED_MS) {
        state = CLOSING;
      }
      break;
    }

    case CLOSING:
      ledcWrite(SERVO_PIN, val_pwm_closed);
      stopLedFade();    // Éteint les LEDs proprement
      state = SLEEP;
      break;
  }
}