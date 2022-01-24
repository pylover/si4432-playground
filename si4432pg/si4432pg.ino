#include <RH_RF22.h>
#include <nash.h>

#define BUSY_LED_PIN 9 
#define SDN_PIN	7 
#define STANDBY_LED_PIN	8 

Nash shell("si4432-# ", BUSY_LED_PIN);
RH_RF22 rf22;


void printbits(uint8_t value) {
	for (int i = 7; i >= 0; i--) {
		PRINT((value >> i) & 1);
	}
}


int parseint(const char *str) {
	uint8_t base = 10;	
	if (str[0] == '0') {
		// May be other bases
		switch (str[1]) {
			case 'b':
			case 'B':
				base = 2;
				break;
			case 'o':
			case 'O':
				base = 8;
				break;
			case 'x':
			case 'X':
				base = 16;
				break;
			default:
				base = 10;
		}
	}
	if (base != 10) {
		str += 2;
	}
	return strtol(str, NULL, base);	
}


int8_t register_set(Nash::Process *self) {
	unsigned char addr = parseint(self->argv[1]);
	unsigned char value = parseint(self->argv[2]);
	rf22.spiWrite(addr, value);
	return EXIT_SUCCESS;
}


int8_t register_get(Nash::Process *self) {
	unsigned char addr = parseint(self->argv[1]);
	uint8_t value = rf22.spiRead(addr);
	
	if (self->argc == 3) {
		char * next;
		uint8_t msb = strtol(self->argv[2], &next, 10);
		uint8_t lsb = strtol(++next, NULL, 10);
		value <<= 7 - msb;
		value >>= (7 - msb) + lsb;
	}
	PRINT("0b");
	printbits(value);
	PRINT(" 0x");
	PRINT(value, HEX);
	PRINT(", decimal: ");
	PRINT(value, DEC);
	PRINTLN();
}


int8_t power(Nash::Process *self) {
	if (self->argc == 2) {
		/* Modify the gpio */
		bool off = self->argv[1][1] == 'f';
		digitalWrite(SDN_PIN, off);
		digitalWrite(STANDBY_LED_PIN, off);
	}

	/* Just report the power status */
	bool status = digitalRead(SDN_PIN);
	PRINT("Power is ");
	PRINTLN(status? "Off": "On");
	return EXIT_SUCCESS;
	
}


int8_t sleep(Nash::Process *self) {
	if (self->signal == SIG_INT) {
		return EXIT_FAILURE;
	}
	unsigned long towait = atol(self->argv[1]) * 1000;
	unsigned long taken = millis() - self->starttime;
	if (taken < towait) {
		return ALIVE;
	}
	return EXIT_SUCCESS;
}


static Nash::Executable programs[] = {
	{"free", 0, 0, NULL, printFreeMemory},
	{"reg-set", 2, 2, "ADDR VALUE", register_set},
	{"reg-get", 1, 2, "ADDR [MSB:LSB]", register_get},
	{"power", 0, 1, "[ON/OFF]", power},
	{"sleep", 1, 1, "NUMBER", sleep},
	{ NULL }
};


void setup() {
	/* GPIO setup */
	pinMode(SDN_PIN, OUTPUT);
	pinMode(STANDBY_LED_PIN, OUTPUT);
	digitalWrite(SDN_PIN, LOW);
	digitalWrite(STANDBY_LED_PIN, LOW);
	
	/* UART */
	Serial.begin(115200);
	
	/* Shell */
	shell.init(programs);
	
	/* RF22 */
	// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, 
	// modulation FSK_Rb2_4Fd36
	if (!rf22.init()) {
		Serial.println("init failed");
	}
}

void loop() {
	shell.loop();
}
