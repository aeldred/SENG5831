#include <pololu/orangutan.h>

/* 

*/

int main() {

	//set for output
	DDRD |= (1 << PIND5);

	//test led	
	PORTD |= (1 << PIND5);
	delay_ms(100);
	PORTD &= ~(1 << PIND5);

	//init timer
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	
	//20000 / 256 = 78
	ICR1 = (uint16_t)(100000);	
	OCR1A = ICR1 / 2;

	TCCR1A |= 0x82;		// configure timer
	//TCCR1B |= 0x1B;		// prescaler 64
	//TCCR1B |= 0x1C;		// prescaler 256
	TCCR1B |= 0x1D;		// prescaler 1024

	while(1) {
		clear();
	}
}

