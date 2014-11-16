/*
 * File:   main.c
 * Author: matt
 *
 * Created on 01 July 2014, 10:39
 */

#include "api.h"


static volatile uint32_t __millis = 0;

#define N_PINS (sizeof(_pins) / sizeof (_pins[0]))

void interrupt low_priority __isr_handler(void) {
    if (INTCONbits.TMR0IE && INTCONbits.T0IF) {
        INTCONbits.T0IF = 0;
        uint16_t tpr = (F_CPU/4)/1000;
        tpr = 0xFFFF - tpr;
        TMR0H = tpr >> 8;
        TMR0L = tpr & 0xFF;
        __millis++;
    }
}

uint32_t millis() {
    uint32_t mil;
    INTCONbits.T0IE = 0;
    mil = __millis;
    INTCONbits.T0IE = 1;
    return mil;
}

void delay(uint32_t del) {
    uint32_t start = millis();
    while (millis() - start < del) {
        savePower();
    }
}

void pinMode(uint8_t pin, uint8_t mode) {
    if (pin >= _pins_max) {
        return;
    }

    switch(mode) {
        case OUTPUT:
            *(_pins[pin].tris) &= (1<<_pins[pin].pin);
            break;

        case INPUT_PULLUP:
            *(_pins[pin].tris) |= (1<<_pins[pin].pin);
            if (_pins[pin].adc >= 0) {
                if (_pins[pin].adc < 8) {
                    ANSEL &= ~(1<<_pins[pin].adc);
                } else {
                    ANSELH &= ~(1<<(_pins[pin].adc-8));
                }
            }
            if (_pins[pin].pullup >= 0) {
                WPUB |= (1<<_pins[pin].pullup);
            }
            break;

        case INPUT:
            *(_pins[pin].tris) |= (1<<_pins[pin].pin);
            if (_pins[pin].adc >= 0) {
                if (_pins[pin].adc < 8) {
                    ANSEL &= ~(1<<_pins[pin].adc);
                } else {
                    ANSELH &= ~(1<<(_pins[pin].adc-8));
                }
            }
            if (_pins[pin].pullup >= 0) {
                WPUB &= ~(1<<_pins[pin].pullup);
            }
            break;
    }
}

void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin >= _pins_max) {
        return;
    }
    if (val > 0) {
        *_pins[pin].lat |= (1<<_pins[pin].pin);
    } else {
        *_pins[pin].lat &= ~(1<<_pins[pin].pin);
    }
}

uint8_t digitalRead(uint8_t pin) {
    if (pin >= _pins_max) {
        return LOW;
    }
    return (*_pins[pin].port & (1<<_pins[pin].pin)) ? HIGH : LOW;
}

void __initSystem() {
    // Configure for 64MHz.
    OSCCONbits.IRCF = 0b111;
    while (OSCCONbits.IOFS == 0) {
        continue;
    }
    OSCTUNEbits.PLLEN = 1;
    // Set up TIMER0 to tick at 1ms intervals.
    // The oscillator ticks at Fosc/4, so 4MHz. That is 1/16000000 s per tick.
    // or 0.000000063s, or 0.000063ms, so 1 ms is 16000 ticks.
    T0CONbits.T08BIT = 0; // 16-bit
    T0CONbits.T0CS = 0; // Internal clock
    T0CONbits.PSA = 1; // No prescaler
    uint16_t tpr = (F_CPU/4)/1000;
    tpr = 0xFFFF - tpr;
    TMR0H = tpr >> 8;
    TMR0L = tpr & 0xFF;
    INTCONbits.T0IF = 0;
    INTCONbits.T0IE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
    T0CONbits.TMR0ON = 1;

    INTCON2bits.RBPU = 0;
    WPUB = 0;

    LATA = 0;
    LATB = 0;
    LATC = 0;
}

// Put the system to sleep until the next "tick" of the timer.  This will
// sleep for up to 1 ms.
void savePower() {
    OSCCONbits.IDLEN = 1;
    OSCCONbits.SCS = 0b00;
    asm("SLEEP");
}

extern void setup();
extern void loop();

void main(void) {
    __initSystem();

    setup();
    while (1) {
        loop();
    }
}
