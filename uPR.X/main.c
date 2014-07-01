#include "api.h"

/*

       A
     F   B
       G
     E   C
       D    P
 */

// UC = left, LC = right
//    FGABfabpcgdePCDE
uint16_t digitsLeft[10] = {
    0b1011000000000111,
    0b0001000000000100,
    0b0111000000000011,
    0b0111000000000110,
    0b1101000000000100,
    0b1110000000000110,
    0b1110000000000111,
    0b1011000000000100,
    0b1111000000000111,
    0b1111000000000110,
};

// UC = left, LC = right
//    FGABfabpcgdePCDE
uint16_t digitsRight[10] = {
    0b0000111010110000,
    0b0000001010000000,
    0b0000011001110000,
    0b0000011011100000,
    0b0000101011000000,
    0b0000110011100000,
    0b0000110011110000,
    0b0000111010000000,
    0b0000111011110000,
    0b0000111011100000,
};

void displayRaw(uint16_t tval) {
    uint8_t tbit;
    for (tbit = 0; tbit < 16; tbit++) {
        digitalWrite(6+tbit, (tval & (1<<tbit)) ? HIGH : 0);
    }
}

void display(uint8_t v) {
    if (v > 99) {
        return;
    }
    uint16_t tval = digitsLeft[v / 10] | digitsRight[v % 10];
    displayRaw(tval);
}

uint8_t ammo = 0;

// Inputs
const uint8_t trigger = 0;
const uint8_t magazine = 1;

// Outputs
const uint8_t fpulse = 4;
const uint8_t ok = 5;

void setup() {
    int i;

    for (i = 7; i < 22; i++) {
        pinMode(i, OUTPUT);
    }
    pinMode(trigger, INPUT_PULLUP);
    pinMode(magazine, INPUT_PULLUP);
    pinMode(fpulse, OUTPUT);
    pinMode(ok, OUTPUT);
    digitalWrite(fpulse, LOW);
    digitalWrite(ok, LOW);
    display(ammo);

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
        displayRaw(0b0000000100000000); // Clear display
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
        displayRaw(0b0000000100000000); // Clear display
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
        display(ammo);
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
        display(ammo);
    }
}
