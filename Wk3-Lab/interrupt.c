#include <pololu/orangutan.h>

/* 
*/

ISR(TIMER0_COMPA_vect) {
	PORTD &= ~(0x04);
	print("INTERRUPT!");
}

int main() {
	//D1 = 0x02  -- red built-in 
	//D2 = 0x04
	//C4 = 0x10 -- green built-in  

	//set for output
	DDRD = 0x04;

	//test led	
	PORTD ^= 0x04;
	PORTD &= ~(0x04);

	//init timer
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;
	
	OCR1A = (uint16_t)(1000);

	TCCR0A |= 0x82;		// configure timer
	TCCR0B |= 0x03;		// prescaler 76mhz
	TIMSK0 |= (1 << OCIE0A); 	//enable the overflow interrupt
	sei();			//allow interrupts

	while(1) {
		PORTD |= 0x04;

	}
}

