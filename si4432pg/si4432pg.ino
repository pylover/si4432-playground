#include <nash.h>
#include <RH_RF22.h>


RH_RF22 rf22;
#define SDN	7 


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


int regset(size_t argc, char **argv, struct process *self) {
	if (argc != 3) {
		ERRORLN("Invalid number of arguments");
		ERRORLN("Usage: regset 0x71 0b10010001");
		return EXIT_FAILURE;
	}
	
	unsigned char addr = parseint(argv[1]);
	unsigned char value = parseint(argv[2]);
	rf22.spiWrite(addr, value);
	return EXIT_SUCCESS;
}


int regget(size_t argc, char **argv, struct process *self) {
	if (argc < 2 || argc > 3) {
		ERRORLN("Invalid number of arguments");
		ERRORLN("Usage: regget 0x71 [MSB:LSB]");
		return EXIT_FAILURE;
	}

	unsigned char addr = parseint(argv[1]);
	uint8_t value = rf22.spiRead(addr);
	
	if (argc == 3) {
		char * next;
		uint8_t msb = strtol(argv[2], &next, 10);
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


int power(size_t argc, char **argv, struct process *self) {
	if (argc > 2) {
		ERRORLN("Invalid number of arguments");
		ERRORLN("Usage: power on|off");
		return EXIT_FAILURE;
	}
	
	digitalWrite(SDN, argv[1][1] == 'o');
	return EXIT_SUCCESS;
}



static struct executable programs[] = {
	{"help", nash_help },
	{"free", nash_free },
	{"regset", regset },
	{"regget", regget },
	{"power", power },
	{ NULL }
};


void setup() {
	nash_init(programs);
	pinMode(SDN, OUTPUT);
	digitalWrite(SDN, LOW);
	// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, 
	// modulation FSK_Rb2_4Fd36
	if (!rf22.init()) {
		Serial.println("init failed");
	}
}

void loop() {
	nash_loop();
}
