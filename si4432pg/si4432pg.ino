#include <RH_RF22.h>
#include <nash.h>


#define BUSY_LED_PIN 9 
#define SDN_PIN	7 
#define STANDBY_LED_PIN	8 

/*
OOK 2.4, 335
1Ch		0xc8	11001000	IF_FILTER_BANDWIDTH
1Fh		0x03	00000011	CLOCK_RECOVERY_GEARSHIFT_OVERRIDE
20h		0x39	00111001    CLOCK_RECOVERY_OVERSAMPLING_RATE
21h		0x20    00100000	...
22h		0x68	01101000	...
23h		0xdc	11011100	...
24h		0x00	00000000	...
25h		0x6b	01101011	...
2ch		0x2a	00101010	OOK_COUNTER_VALUE_1
2dh		0x08	00001000	...
2eh		0x2a	00101010	...
58h		0x80	10000000	CHARGE_PUMP_CURRENT_TRIMMING
69h		0x60	01100000	AGC_OVERRIDE1
6eh		0x13	00010011	TX_DATA_RATE1
6fh		0xa9	10101001	...
70h		0x2c	00101100	...
71h		0x21	00100001	...
72h		0x08	00001000	...
*/

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


int8_t freq(Nash::Process *self) {
	float freq = atof(self->argv[1]);
	rf22.setFrequency(freq);
	PRINT("Frequency has been set to: ");
	PRINTLN(freq);
	return EXIT_SUCCESS;
}

int8_t send(Nash::Process *self) {
	if (self->signal == SIG_INT) {
		return EXIT_FAILURE;
	}

	rf22.send(self->argv[1], strlen(self->argv[1]));
	return EXIT_SUCCESS;
}

int8_t recv(Nash::Process *self) {
	if (self->signal == SIG_INT) {
		return EXIT_FAILURE;
	}
	
	uint8_t amount = self->argc > 1? atoi(self->argv[1]): 8;
	char buff[amount + 1];
	if (!rf22.recv(buff, &amount)) {
		return ALIVE;
	}
	buff[amount] = NULL;
	PRINTLN(buff);
	return EXIT_SUCCESS;
}

int8_t jam(Nash::Process *self) {
	if (self->signal == SIG_INT) {
		return EXIT_FAILURE;
	}
	
	rf22.send("abcdefghijklmnopqrstuvwxyzabcdef", 32);
	return ALIVE;
}

int8_t listen(Nash::Process *self) {
	if (self->signal == SIG_INT) {
		return EXIT_FAILURE;
	}
	
	uint8_t amount = 32;
	char buff[amount + 1];
	if (rf22.recv(buff, &amount)) {
		buff[amount] = NULL;
		PRINTLN(buff);
	}
	return ALIVE;
}



static Nash::Executable programs[] = {
	{"free",    0, 0, NULL,             printFreeMemory   },
	{"reg-set", 2, 2, "ADDR VALUE",     register_set      },
	{"reg-get", 1, 2, "ADDR [MSB:LSB]", register_get      },
	{"power",   0, 1, "[ON/OFF]",       power             },
	{"freq",    1, 1, "FREQUENCY",      freq              },
	{"recv",    0, 1, "[BYTES]",        recv              },
	{"send",    1, 1, "DATA",           send              },
	{"jam",     0, 0, NULL,             jam               },
	{"listen",  0, 0, NULL,             listen            },
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
	
	rf22.setFrequency(433.92, 0.05);
	//rf22.setModemConfig(RH_RF22::OOK_Rb1_2Bw75);
	//rf22.setModemConfig(RH_RF22::OOK_Rb2_4Bw335);
	//rf22.setModemConfig(RH_RF22::OOK_Rb4_8Bw335);
	//rf22.setModemConfig(RH_RF22::OOK_Rb40Bw335);
	rf22.setModemConfig(RH_RF22::GFSK_Rb125Fd125);
    rf22.setTxPower(RH_RF22_TXPOW_20DBM);

}


void loop() {
	shell.loop();
}
