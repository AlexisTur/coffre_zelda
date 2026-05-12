#include <Arduino.h>
#include "driver/rtc_io.h"

// =========================
// Configuration
// =========================

constexpr gpio_num_t WAKEUP_GPIO = GPIO_NUM_5;
constexpr uint8_t LED_PIN = 48;

constexpr uint32_t TIME_WINDOW_MS = 3000;
constexpr uint32_t DEBOUNCE_MS    = 200;
constexpr uint8_t REQUIRED_PULSES = 3;

// =========================
// Variables
// =========================

uint8_t pulseCount = 0;

unsigned long startTime      = 0;
unsigned long lastPulseTime  = 0;

// =========================
// Fonctions
// =========================

void goToSleep(const char* reason)
{
    Serial.println(reason);

    // Réveil sur niveau HIGH du GPIO 5
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 1);

    delay(100);
    esp_deep_sleep_start();
}

bool pulseDetected()
{
    static bool previousState = false;

    bool currentState = digitalRead(WAKEUP_GPIO);

    // Front montant
    if (currentState && !previousState)
    {
        unsigned long now = millis();

        // Debounce
        if ((now - lastPulseTime) > DEBOUNCE_MS)
        {
            lastPulseTime = now;
            previousState = currentState;
            return true;
        }
    }

    previousState = currentState;
    return false;
}

void blinkLed()
{
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(LED_PIN, LOW);
}

// =========================
// Setup
// =========================

void setup()
{
    Serial.begin(115200);

    pinMode(WAKEUP_GPIO, INPUT);
    pinMode(LED_PIN, OUTPUT);

    startTime = millis();

    // Première impulsion = réveil
    pulseCount = 1;

    Serial.println("Wakeup");
}

// =========================
// Loop
// =========================

void loop()
{
    unsigned long now = millis();

    // Timeout : pas assez d'impulsions
    if ((now - startTime) >= TIME_WINDOW_MS)
    {
        goToSleep("Timeout -> Sleep");
    }

    // Nouvelle impulsion détectée
    if (pulseDetected())
    {
        pulseCount++;

        Serial.print("Pulse count: ");
        Serial.println(pulseCount);
    }

    // 3 impulsions reçues
    if (pulseCount >= REQUIRED_PULSES)
    {
        Serial.println("3 pulses detected");

        blinkLed();

        goToSleep("Done -> Sleep");
    }
}