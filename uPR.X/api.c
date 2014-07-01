/*
 * File:   main.c
 * Author: matt
 *
 * Created on 01 July 2014, 10:39
 */

#include "api.h"

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1H
#pragma config FOSC = INTIO67   // Oscillator Selection bits (Internal oscillator block, port function on RA6 and RA7)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
#pragma config BORV = 18        // Brown Out Reset Voltage bits (VBOR set to 1.8 V nominal)

// CONFIG2H
#pragma config WDTEN = OFF      // Watchdog Timer Enable bit (WDT is controlled by SWDTEN bit of the WDTCON register)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = PORTC   // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = ON      // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config HFOFST = ON      // HFINTOSC Fast Start-up (HFINTOSC starts clocking the CPU without waiting for the oscillator to stablize.)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection Block 0 (Block 0 (000200-000FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection Block 1 (Block 1 (001000-001FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0001FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection Block 0 (Block 0 (000200-000FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection Block 1 (Block 1 (001000-001FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0001FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection Block 0 (Block 0 (000200-000FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection Block 1 (Block 1 (001000-001FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0001FFh) not protected from table reads executed in other blocks)

static volatile uint32_t __millis = 0;

struct ioPin {
    volatile uint8_t *tris;
    volatile uint8_t *port;
    volatile uint8_t *lat;
    uint8_t pin;
    int8_t adc;
    int8_t pullup;
};

const struct ioPin _pins[] = {
    { &TRISB, &PORTB, &LATB, 7, -1, 7 },
    { &TRISB, &PORTB, &LATB, 6, -1, 6 },
    { &TRISB, &PORTB, &LATB, 0, 12, 0 },
    { &TRISB, &PORTB, &LATB, 1, 10, 1 },
    { &TRISA, &PORTA, &LATA, 0, 0, -1 },
    { &TRISA, &PORTA, &LATA, 1, 1, -1 },
    { &TRISC, &PORTC, &LATC, 7, -1, -1 },
    { &TRISC, &PORTC, &LATC, 6, -1, -1 },
    { &TRISC, &PORTC, &LATC, 5, -1, -1 },
    { &TRISC, &PORTC, &LATC, 4, -1, -1 },
    { &TRISC, &PORTC, &LATC, 3, -1, -1 },
    { &TRISC, &PORTC, &LATC, 2, -1, -1 },
    { &TRISC, &PORTC, &LATC, 1, -1, -1 },
    { &TRISC, &PORTC, &LATC, 0, -1, -1 },
    { &TRISA, &PORTA, &LATA, 5, 4, -1 },
    { &TRISA, &PORTA, &LATA, 4, -1, -1 },
    { &TRISA, &PORTA, &LATA, 3, 3, -1 },
    { &TRISA, &PORTA, &LATA, 2, 2, -1 },
    { &TRISB, &PORTB, &LATB, 5, -1, -1 },
    { &TRISB, &PORTB, &LATB, 4, 11, -1 },
    { &TRISB, &PORTB, &LATB, 3, 9, -1 },
    { &TRISB, &PORTB, &LATB, 2, 8, -1 },
};

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
        continue;
    }
}

void pinMode(uint8_t pin, uint8_t mode) {
    if (pin >= N_PINS) {
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
    if (pin >= N_PINS) {
        return;
    }
    if (val > 0) {
        *_pins[pin].lat |= (1<<_pins[pin].pin);
    } else {
        *_pins[pin].lat &= ~(1<<_pins[pin].pin);
    }
}

uint8_t digitalRead(uint8_t pin) {
    if (pin >= N_PINS) {
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

extern void setup();
extern void loop();

void main(void) {
    __initSystem();

    setup();
    while (1) {
        loop();
    }
}
