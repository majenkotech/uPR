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
const uint8_t ee_single = 7;
const uint8_t ee_updown = 8;
const uint8_t ee_mfunc = 9;
const uint8_t ee_debounce = 10;

uint8_t duty = 10;
uint8_t start = 99;
uint8_t down = 95;
uint8_t speed1 = 250;
uint8_t speed2 = 66;
uint8_t pulse = 50;
uint8_t single = 0;
uint8_t updown = 0;
uint8_t mfunc = 0;
uint8_t debounce = 30;

enum {
    IDLE,
    LOADING,
    ACTIVE,
    BLOCKED
};

uint8_t trigState = IDLE;
uint8_t magState = IDLE;
uint8_t ammoState = IDLE;

void setup() {
    uint8_t isConfigured = 0;
    
	Serial.begin(115200);
	LED.setPins(ledPins);
	pinMode(trigger, INPUT_PULLUP);
	pinMode(magazine, INPUT_PULLUP);
	pinMode(fpulse, OUTPUT);
	pinMode(ok, OUTPUT);
	digitalWrite(fpulse, LOW);
	digitalWrite(ok, LOW);


	isConfigured = EEPROM.read(ee_first);
	if (isConfigured != 1) {
		EEPROM.write(ee_duty, duty);
		EEPROM.write(ee_first, 1);
		EEPROM.write(ee_start, start);
		EEPROM.write(ee_down, down);
		EEPROM.write(ee_speed1, speed1);
		EEPROM.write(ee_speed2, speed2);
		EEPROM.write(ee_pulse, pulse);
        EEPROM.write(ee_single, single);
        EEPROM.write(ee_updown, updown);
        EEPROM.write(ee_mfunc, mfunc);
        EEPROM.write(ee_debounce, debounce);
	}

	duty = EEPROM.read(ee_duty);
	start = EEPROM.read(ee_start);
	down = EEPROM.read(ee_down);
	speed1 = EEPROM.read(ee_speed1);
	speed2 = EEPROM.read(ee_speed2);
	pulse = EEPROM.read(ee_pulse);
    single = EEPROM.read(ee_single);
    updown = EEPROM.read(ee_updown);
    mfunc = EEPROM.read(ee_mfunc);
    debounce = EEPROM.read(ee_debounce);
    if (digitalRead(magazine) == LOW) {
    	ammo = start;
        ammoState = LOADING;
        magState = ACTIVE;
    } else {
        ammo = 0;
        ammoState = IDLE;
        magState = IDLE;
    }

    if (mfunc == 1) { // Ammo reset function
        ammo = start;
        ammoState = LOADING;
    }
}

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
    Serial.print_P((const char *)"[S/s] Single-shot mode: ");  
    if (single == 1) {
        Serial.println_P((const char *)"on");
    } else {
        Serial.println_P((const char *)"off");
    }
    Serial.print_P((const char *)"[D/d] Count direction: ");  
    if (updown == 1) {
        Serial.println_P((const char *)"up");
    } else {
        Serial.println_P((const char *)"down");
    }
    Serial.print_P((const char *)"[M/m] MAG button function: ");  
    if (mfunc == 0) {
        Serial.println_P((const char *)"magazine");
    } else if (mfunc == 1) {
        Serial.println_P((const char *)"ammo reset");
    }
    Serial.print_P((const char *)"[O/o] Button debounce time: ");     sprintf(temp, "%u", debounce);      Serial.print(temp); Serial.println_P((const char *)"ms");
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

int debouncedTrigger() {
    static uint32_t lastChange = 0;
    static int savedValue = HIGH;
    static int debouncedValue = HIGH;
    
    int currentValue = digitalRead(trigger);
    if (currentValue != savedValue) { // The level changed
        savedValue = currentValue;
        lastChange = millis();
    }

    if (lastChange > 0) {
        if (millis() - lastChange >= debounce) {
            debouncedValue = savedValue;
            lastChange = 0;
        }
    }

    return debouncedValue;
}

int debouncedMagazine() {
    static uint32_t lastChange = 0;
    static int savedValue = HIGH;
    static int debouncedValue = HIGH;
    
    int currentValue = digitalRead(magazine);
    if (currentValue != savedValue) { // The level changed
        savedValue = currentValue;
        lastChange = millis();
    }

    if (lastChange > 0) {
        if (millis() - lastChange >= debounce) {
            debouncedValue = savedValue;
            lastChange = 0;
        }
    }

    return debouncedValue;
}

void serialMenu() {
    static uint32_t saveTimer = 0;

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
            case 'D': changed = tweak(&updown, 0, 1, +1); break;
            case 'd': changed = tweak(&updown, 0, 1, -1); break;
            case 'S': changed = tweak(&single, 0, 1, +1); break;
            case 's': changed = tweak(&single, 0, 1, -1); break;
            case 'M': changed = tweak(&mfunc, 0, 1, +1); break;
            case 'm': changed = tweak(&mfunc, 0, 1, -1); break;
            case 'O': changed = tweak(&debounce, 0, 100, +1); break;
            case 'o': changed = tweak(&debounce, 0, 100, -1); break;
            case 'r':
                asm ("RESET");
                while(1);
                break;
        }
        if (changed != 0) {
            saveTimer = millis();
        }
        displaySettings();
    }

    if (saveTimer > 0) {
        if (millis() - saveTimer > 5000) {
            EEPROM.write(ee_duty, duty);
            EEPROM.write(ee_start, start);
            EEPROM.write(ee_down, down);
            EEPROM.write(ee_speed1, speed1);
            EEPROM.write(ee_speed2, speed2);
            EEPROM.write(ee_single, single);
            EEPROM.write(ee_updown, updown);
            EEPROM.write(ee_mfunc, mfunc);
            saveTimer = 0;
            Serial.println_P((const char *)"Saved");
        }
    }

}

void PWMDisplay() {
    static uint8_t phase = 0;

    if (phase < duty) {
        if (updown == 1) {
            LED.displayValue(start - ammo);
        } else {
            LED.displayValue(ammo);
        }
    } else {
        LED.displayRaw(0);
    }
    phase++;
    if (phase >= 10) {
        phase = 0;
    }

}

void loop() {
	static uint8_t state = LOADING;
	static uint32_t ts = 0;
	static uint32_t pulseCounter = 0;
	static uint32_t triggerWentLow = 0;

	delay(1);

    serialMenu();
    PWMDisplay();
    
	if (pulseCounter > 0) {
		if (millis() - pulseCounter > pulse) {
			digitalWrite(fpulse, LOW);
			pulseCounter = 0;			
		}
	}	

    if (mfunc == 0) { // Normal magazine operation       
        if (debouncedMagazine() == LOW && magState == IDLE) { // Inserted
            ammoState = LOADING;
            ammo = start;
            magState = ACTIVE;
            ts = millis();
        }

        if (debouncedMagazine() == HIGH && magState == ACTIVE) { // Removed
            ammoState = IDLE;
            ammo = 0;
            digitalWrite(ok, LOW);
            magState = IDLE;
        }
    } else if (mfunc == 1) { // Simple ammo reset
        if (debouncedMagazine() == LOW && magState == IDLE) { // Pressed
            ammoState = ACTIVE;
            ammo = start;
        }

        if (debouncedMagazine() == HIGH && magState == ACTIVE) { // Released
            // Nothing is done here.
        }
    }

	if (debouncedTrigger() == LOW && trigState == IDLE) {
		trigState = ACTIVE;
		ts = millis();
	}

    if (debouncedTrigger() == HIGH && trigState == ACTIVE) {
        trigState = IDLE;
    }

    if (debouncedTrigger() == HIGH && trigState == BLOCKED) {
        trigState = IDLE;
    }


    if (ammoState == LOADING) {
		if (millis() - ts > speed1) {
			ts = millis();

			if (ammo == down) {
				digitalWrite(ok, HIGH);
				ammoState = ACTIVE;
			} else {
                ammo--;
			}
		}
	}

    if (ammoState == ACTIVE) {
        if (trigState == ACTIVE) {
            if (single == 1) {
                trigState = BLOCKED;
                ammo--;
                pulseCounter = millis();
    
                if (ammo == 0) {
                    ammoState = IDLE;
                    digitalWrite(ok, LOW);
                }                
            } else {
                if (millis() - ts > speed2) {
                    digitalWrite(fpulse, HIGH);
                    pulseCounter = millis();
                    ts = millis();
                    ammo--;
    
                    if (ammo == 0) {
                        ammoState = IDLE;
                        digitalWrite(ok, LOW);
                    }                
                }
            }
        }
    }
}
