#ifndef API_H
#define	API_H

#ifdef	__cplusplus
extern "C" {
#endif


#include <xc.h>
#include <stdint.h>

#define F_CPU 64000000UL
#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
#define LOW 0
#define HIGH 1

extern uint32_t millis();
extern void delay(uint32_t del);
extern void pinMode(uint8_t pin, uint8_t mode);
extern void digitalWrite(uint8_t pin, uint8_t val);
extern uint8_t digitalRead(uint8_t pin);

#ifdef	__cplusplus
}
#endif

#endif	/* API_H */

