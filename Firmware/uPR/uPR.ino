#include "LEDDisplay.h"
#include "EEPROM.h"

uint8_t ammo = 0;

// Inputs
const uint8_t trigger = PIN_TRIG;
const uint8_t magazine = PIN_MAG;

// Outputs
const uint8_t fpulse = 2;
const uint8_t ok = 3;

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

const uint8_t ee_first = 0;
const uint8_t ee_duty = 1;
const uint8_t ee_start = 2;
const uint8_t ee_down = 3;
const uint8_t ee_speed1 = 4;
const uint8_t ee_speed2 = 5;
const uint8_t ee_pulse = 6;

uint8_t duty = 10;
uint8_t start = 99;
uint8_t down = 95;
uint8_t speed1 = 250;
uint8_t speed2 = 66;
uint8_t pulse = 50;

void setup() {
	Serial.begin(115200);
	LED.setPins(ledPins);
	pinMode(trigger, INPUT_PULLUP);
	pinMode(magazine, INPUT_PULLUP);
	pinMode(fpulse, OUTPUT);
	pinMode(ok, OUTPUT);
	digitalWrite(fpulse, LOW);
	digitalWrite(ok, LOW);


	uint8_t isConfigured = EEPROM.read(ee_first);
	if (isConfigured != 1) {
		EEPROM.write(ee_duty, duty);
		EEPROM.write(ee_first, 1);
		EEPROM.write(ee_start, start);
		EEPROM.write(ee_down, down);
		EEPROM.write(ee_speed1, speed1);
		EEPROM.write(ee_speed2, speed2);
		EEPROM.write(ee_pulse, pulse);
	}

	duty = EEPROM.read(ee_duty);
	start = EEPROM.read(ee_start);
	down = EEPROM.read(ee_down);
	speed1 = EEPROM.read(ee_speed1);
	speed2 = EEPROM.read(ee_speed2);
	pulse = EEPROM.read(ee_pulse);
	ammo = start;
}

enum {
	BOOT,
	IDLE,
	LOADING,
	FIRING,
	EMPTY,
	REMOVED,
};

void displaySettings() {
	char temp[5];
	Serial.print_P((const char *)"\x1b[2J\x1b[1;1H");
	Serial.println_P((const char *)"uPR Settings");
	Serial.println_P((const char *)"============");
	Serial.println_P((const char *)"");

	Serial.print_P((const char *)"[B/b] Brightness: ");		sprintf(temp, "%u", duty);		Serial.print(temp);	Serial.println_P((const char *)"");
	Serial.print_P((const char *)"[L/l] Load speed: ");	sprintf(temp, "%u", speed1);	Serial.print(temp);	Serial.println_P((const char *)"ms");
	Serial.print_P((const char *)"[F/f] Fire speed: ");		sprintf(temp, "%u", speed2);	Serial.print(temp);	Serial.println_P((const char *)"ms");
	Serial.print_P((const char *)"[I/i] Initial ammo: ");	sprintf(temp, "%u", start);		Serial.print(temp);	Serial.println_P((const char *)"");
	Serial.print_P((const char *)"[C/c] Countdown ammo: ");	sprintf(temp, "%u", down);		Serial.print(temp);	Serial.println_P((const char *)"");
	Serial.print_P((const char *)"[P/p] Output pulse: ");	sprintf(temp, "%u", pulse);		Serial.print(temp);	Serial.println_P((const char *)"ms");
	Serial.println_P((const char *)"");
}

uint8_t tweak(uint8_t *val, uint8_t min, uint8_t max, int8_t dir) {
	if ((dir == -1) && (*val <= min)) {
		*val = min;
		return 0;
	}
	if ((dir == 1) && (*val >= max)) {
		*val = max;
		return 0;
	}

	*val += dir;
	return 1;
}

void loop() {
	static uint8_t state = BOOT;
	static uint32_t ts = 0;
	static uint8_t phase = 0;
	static uint32_t saveCounter = 0;
	static uint32_t pulseCounter = 0;
	
	delay(1);

	if (saveCounter > 0) {
		if (millis() - saveCounter > 5000) {
			EEPROM.write(ee_duty, duty);
			EEPROM.write(ee_start, start);
			EEPROM.write(ee_down, down);
			EEPROM.write(ee_speed1, speed1);
			EEPROM.write(ee_speed2, speed2);
			saveCounter = 0;
			Serial.println_P((const char *)"Saved");
		}
	}

	if (pulseCounter > 0) {
		if (millis() - pulseCounter > pulse) {
			digitalWrite(fpulse, LOW);
			pulseCounter = 0;			
		}
	}
	
	if (Serial.available()) {
		int sr = Serial.read();
		uint8_t changed = 0;
		switch(sr) {
			case 'B': changed = tweak(&duty, 0, 10, +1); break;
			case 'b': changed = tweak(&duty, 0, 10, -1); break;
			case 'L': changed = tweak(&speed1, 0, 255, +1); break;
			case 'l': changed = tweak(&speed1, 0, 255, -1); break;
			case 'F': changed = tweak(&speed2, 0, 255, +1); break;
			case 'f': changed = tweak(&speed2, 0, 255, -1); break;
			case 'I': changed = tweak(&start, 0, 99, +1); break;
			case 'i': changed = tweak(&start, 0, 99, -1); break;
			case 'C': changed = tweak(&down, 0, 99, +1); break;
			case 'c': changed = tweak(&down, 0, 99, -1); break;
			case 'P': changed = tweak(&pulse, 0, 255, +1); break;
			case 'p': changed = tweak(&pulse, 0, 255, -1); break;
			case 'r':
				asm ("RESET");
				while(1);
				break;
		}
		if (changed != 0) {
			saveCounter = millis();
		}
		displaySettings();
	}

	if (phase < duty) {
		LED.displayValue(ammo);
	} else {
		LED.displayRaw(0);
	}
	phase++;
	if (phase >= 10) {
		phase = 0;
	}
	
	if (digitalRead(magazine) == HIGH && state == BOOT) {
		state = REMOVED;
		ts = millis();
		digitalWrite(ok, LOW);
		ammo = 0;
	}

	if (digitalRead(magazine) == LOW && state == BOOT) {
		state = LOADING;
		ts = millis();
		digitalWrite(ok, LOW);
		ammo = start;
	}

	// Magazine has just been removed.
	if (digitalRead(magazine) == HIGH && state != REMOVED) {
		state = REMOVED;
		ammo = 0;
		digitalWrite(ok, LOW);
	}

	if (digitalRead(magazine) == LOW && state == REMOVED) {
		state = LOADING;
		ammo = start;
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
		if (millis() - ts > speed1) {
			ts = millis();

			if (ammo == down) {
				digitalWrite(ok, HIGH);
				state = IDLE;                
			} else {
                ammo--;
			}
		}
	}

	if (state == FIRING) {
		if (millis() - ts > speed2) {
			digitalWrite(fpulse, HIGH);
			pulseCounter = millis();
			ts = millis();

			if (ammo == 0) {
				digitalWrite(ok, LOW);
				state = EMPTY;
			} else {
                ammo--;
			}
		}
	}
}
