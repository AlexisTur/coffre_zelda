#include <Arduino.h>
#include "driver/rtc_io.h"
#include "HardwareSerial.h"
#include "DFRobotDFPlayerMini.h"

// =========================
// Configuration
// =========================

constexpr gpio_num_t WAKEUP_GPIO      = GPIO_NUM_5;
constexpr uint8_t    EN_PIN           = 12;
constexpr uint8_t    SERVO_PIN        = 13;

constexpr uint32_t   TIME_WINDOW_MS   = 5000;
constexpr uint32_t   DEBOUNCE_MS      = 600;
constexpr uint8_t    REQUIRED_PULSES  = 4;
constexpr uint32_t   TIME_OPENED_MS   = 10000;

constexpr uint32_t   SOUND_VOLUME   = 15;

// =========================
// DFPlayer
// =========================

constexpr uint8_t RXD2 = 7;
constexpr uint8_t TXD2 = 6;

HardwareSerial       dfSD(1);
DFRobotDFPlayerMini  player;

// =========================
// LEDC / LED Fade
// =========================

constexpr uint8_t  LEDC_TIMER_BITS       = 12;
constexpr uint32_t LEDC_BASE_FREQ        = 5000;
constexpr uint8_t  LED_PIN_BLUE          = 10;
constexpr uint8_t  LED_PIN_RED           = 11;

constexpr uint32_t LEDC_START_DUTY_BLUE  = 1000;
constexpr uint32_t LEDC_TARGET_DUTY_BLUE = 4095;
constexpr uint32_t LEDC_START_DUTY_RED   = 330;
constexpr uint32_t LEDC_TARGET_DUTY_RED  = 1500;
constexpr uint32_t LEDC_FADE_TIME        = 2000;

// =========================
// Servo
// =========================

constexpr uint32_t SERVO_FREQ       = 50;
constexpr uint8_t  SERVO_RESOLUTION = 14;
constexpr int      PWM_CLOSED       = 14300;
constexpr int      PWM_OPENED       = 15100;

// =========================
// State machine
// =========================

enum class State : uint8_t {
  SLEEP,
  VERIF,
  INIT,
  OPENING,
  WAIT,
  CLOSING
};

// =========================
// Globals
// =========================

volatile bool fade_ended  = false;
bool          fade_in     = true;
bool          leds_running = false;

uint8_t       pulseCount   = 0;
unsigned long startTime    = 0;
unsigned long lastPulseTime = 0;

State state = State::VERIF;

// =========================
// ISR
// =========================

void ARDUINO_ISR_ATTR onLedFadeEnd() {
  fade_ended = true;
}

// =========================
// MP3 Player
// =========================

void initMP3Player() {
  dfSD.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  Serial.println("Init DFPlayer...");
  if (player.begin(dfSD, false)) {
    Serial.println("DFPlayer OK");
    player.volume(SOUND_VOLUME);
  } else {
    Serial.println("DFPlayer init failed!");
  }
}

// =========================
// LED Fade
// =========================

void startLedFade() {
  leds_running = true;
  fade_in      = true;
  fade_ended   = true; // Déclenche updateLedFade() dès le premier appel

  ledcAttach(LED_PIN_BLUE, LEDC_BASE_FREQ, LEDC_TIMER_BITS);
  ledcAttach(LED_PIN_RED,  LEDC_BASE_FREQ, LEDC_TIMER_BITS);
}

void stopLedFade() {
  leds_running = false;
  fade_ended   = false;

  ledcWrite(LED_PIN_BLUE, 0);
  ledcWrite(LED_PIN_RED,  0);
  ledcDetach(LED_PIN_BLUE);
  ledcDetach(LED_PIN_RED);
}

void updateLedFade() {
  if (!leds_running || !fade_ended) return;

  fade_ended = false;

  if (fade_in) {
    ledcFadeWithInterrupt(LED_PIN_BLUE, LEDC_START_DUTY_BLUE, LEDC_TARGET_DUTY_BLUE, LEDC_FADE_TIME, onLedFadeEnd);
    ledcFadeWithInterrupt(LED_PIN_RED,  LEDC_START_DUTY_RED,  LEDC_TARGET_DUTY_RED,  LEDC_FADE_TIME, onLedFadeEnd);
  } else {
    ledcFadeWithInterrupt(LED_PIN_BLUE, LEDC_TARGET_DUTY_BLUE, LEDC_START_DUTY_BLUE, LEDC_FADE_TIME, onLedFadeEnd);
    ledcFadeWithInterrupt(LED_PIN_RED,  LEDC_TARGET_DUTY_RED,  LEDC_START_DUTY_RED,  LEDC_FADE_TIME, onLedFadeEnd);
  }

  fade_in = !fade_in;
}

// =========================
// Sleep
// =========================

void goToSleep(const char* reason) {
  Serial.println(reason);
  esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);
  delay(100);
  esp_deep_sleep_start();
}

// =========================
// Pulse detection
// =========================

bool pulseDetected() {
  static bool previousState = false;
  bool currentState = digitalRead(WAKEUP_GPIO);

  if (currentState && !previousState) {
    unsigned long now = millis();
    if ((now - lastPulseTime) > DEBOUNCE_MS) {
      lastPulseTime  = now;
      previousState  = currentState;
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

    case State::SLEEP:
      goToSleep(pulseCount >= REQUIRED_PULSES ? "Done -> Sleep" : "Timeout -> Sleep");
      break;

    case State::VERIF:
    {
      if ((millis() - startTime) >= TIME_WINDOW_MS) {
        state = State::SLEEP;
        break;
      }

      if (pulseDetected()) {
        pulseCount++;
        Serial.print("Pulse count: ");
        Serial.println(pulseCount);
      }

      if (pulseCount >= REQUIRED_PULSES) {
        Serial.println(String(REQUIRED_PULSES) + " pulses detected");
        state = State::INIT;
      }
      break;
    }

    case State::INIT:
      digitalWrite(EN_PIN, HIGH);
      initMP3Player();
      startLedFade();
      ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION);
      state = State::OPENING;
      break;

    case State::OPENING:
      Serial.println("Playing #1");
      player.play(1);
      delay(1000);
      updateLedFade();
      delay(1600);
      ledcWrite(SERVO_PIN, PWM_OPENED);
      startTime = millis();
      state = State::WAIT;
      break;

    case State::WAIT:
      updateLedFade();
      if ((millis() - startTime) >= TIME_OPENED_MS) {
        state = State::CLOSING;
      }
      break;

    case State::CLOSING:
      ledcWrite(SERVO_PIN, PWM_CLOSED);
      stopLedFade();
      state = State::SLEEP;
      break;
  }
}