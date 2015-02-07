#include <avr/io.h>

int main() {
	//D3 = register 4 = 0x08
	DDRD = 0x04;
	while(1) {
		PORTD |= 0x04;
		//PORTD = 1 << PORTD2;

	}
}

