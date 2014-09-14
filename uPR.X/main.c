#include "api.h"
#include "LEDDisplay.h"


uint8_t ammo = 0;

// Inputs
const uint8_t trigger = PIN_TRIG;
const uint8_t magazine = PIN_MAG;

// Outputs
const uint8_t fpulse = 4;
const uint8_t ok = 5;

// LED pins:
//
// Left digit:
//
// A = 19
// B = 18
// C = 8
// D = 7
// E = 6
// F = 21
// G = 20
// DP = 9
//
// Right digit:
//
// A = 16
// B = 15
// C = 13
// D = 11
// E = 10
// F = 17
// G = 12
// DP = 14

const uint8_t ledPins[16] = {
    19, 18, 8, 7, 6, 21, 20, 9,
    16, 15, 13, 11, 10, 17, 12, 14
};

void setup() {
    LED.setPins(ledPins);
    pinMode(trigger, INPUT_PULLUP);
    pinMode(magazine, INPUT_PULLUP);
    pinMode(fpulse, OUTPUT);
    pinMode(ok, OUTPUT);
    digitalWrite(fpulse, LOW);
    digitalWrite(ok, LOW);

    LED.displayValue(ammo);

}

enum {
    BOOT,
    IDLE,
    LOADING,
    FIRING,
    EMPTY,
    REMOVED,
};

void loop() {
    static uint8_t state = BOOT;
    static uint32_t ts = 0;

    if (digitalRead(magazine) == HIGH && state == BOOT) {
        state = REMOVED;
        ts = millis();
        digitalWrite(ok, LOW);
        ammo = 0;
        LED.displayValue(ammo);
    }

    if (digitalRead(magazine) == LOW && state == BOOT) {
        state = LOADING;
        ts = millis();
        digitalWrite(ok, LOW);
        ammo = 99;
    }

    // Magazine has just been removed.
    if (digitalRead(magazine) == HIGH && state != REMOVED) {
        state = REMOVED;
        ammo = 0;
        LED.displayValue(ammo);
        digitalWrite(ok, LOW);
    }

    if (digitalRead(magazine) == LOW && state == REMOVED) {
        state = LOADING;
        ammo = 99;
        ts = millis();
        digitalWrite(ok, LOW);
    }

    if (digitalRead(trigger) == LOW && state == IDLE) {
        state = FIRING;
        ts = millis();
    }

    if (digitalRead(trigger) == HIGH && state == FIRING) {
        state = IDLE;
    }

    if (state == LOADING) {
        if (millis() - ts > 250) {
            ts = millis();
            ammo--;
            if (ammo == 95) {
                digitalWrite(ok, HIGH);
                state = IDLE;
            }
        }
        LED.displayValue(ammo);
    }

    if (state == FIRING) {
        if (millis() - ts > 66) {
            digitalWrite(fpulse, HIGH);
            ts = millis();
            ammo--;
            if  (ammo == 0) {
                digitalWrite(ok, LOW);
                state = EMPTY;
            }
            digitalWrite(fpulse, LOW);
        }
        LED.displayValue(ammo);
    }
}
