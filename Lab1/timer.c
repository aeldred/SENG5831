#include <avr/interrupt.h>
#include <pololu/orangutan.h>

#include <stdio.h>
#include "timer.h"
#include "led.h"

//globals
long f_IO = 20000000;

int R_PRESCALER = 256;
int Y_PRESCALER = 64;
int G_PRESCALER = 1024;

extern uint32_t Y_TICKS;
extern uint32_t G_TICKS;
extern uint32_t MS_TICKS;

extern uint16_t RELEASE_RED;

extern uint16_t R_TOGGLE_FREQ;
extern uint16_t Y_TOGGLE_FREQ;
extern uint16_t G_TOGGLE_FREQ;

ISR(TIMER0_COMPA_vect) {
	// This is the Interrupt Service Routine for Timer0 (10ms clock used for scheduling red).
	// Each time the TCNT count is equal to the OCR0 register, this interrupt is "fired".
	// Increment ticks
	MS_TICKS++;

	
	// if time to toggle the RED LED, set flag to release
	if ( ( MS_TICKS % (R_TOGGLE_FREQ)) == 0 ) {
		toggle(RED);
		MS_TICKS = 0;
	}
}

ISR(TIMER3_COMPA_vect) {
	sei();
	int i;
	for (i = 0; i < 51; i++) {
		WAIT_10MS;
	}
	Y_TICKS++;

	if (Y_TICKS == Y_TOGGLE_FREQ) {
		toggle(YELLOW);
		Y_TICKS = 0;
	}
}

ISR(TIMER1_COMPA_vect) {
	//increments green toggle counter
	toggle(GREEN);
}

void init_timers() {
	
	//---------------RED---------------------
        TCCR0A = 0;
        TCCR0B = 0;
        TCNT0 = 0;
	
	TCCR0A |= 0x82;
	TCCR0B |= 0x04;
        TIMSK0 |= (1 << OCIE0A);        
	OCR0A = (uint8_t)(77);
	MS_TICKS = 0;

	//-------------YELLOW--------------------
	TCCR3A = 0;
	TCCR3B = 0;
	TCCR3C = 0;
	TCNT3 = 0;

        TCCR3A |= 0x40;         
        TCCR3B |= 0x0B;        
       	TCCR3C |= 0x00;
        TIMSK3 |= (1 << OCIE3A);        
	OCR3A = calculate_OCRA(YELLOW);
	Y_TICKS = 0;

	//--------------GREEN--------------------
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1C = 0;
	TCNT1 = 0;

	TCCR1A |= 0x43;
	TCCR1B |= 0x1D;
	TCCR1C |= 0x00;
	TIMSK1 |= (1 << OCIE1A);
	OCR1A = calculate_OCRA(GREEN);
	OCR1B = 1;
	G_TICKS = 0;	
}


int calculate_OCRA (led) {
	int HZ;
	int step1 = 1;
	int step2 = 1;
	switch(led) {
		case GREEN:
			HZ = 1000/(G_TIMER_RES*G_TOGGLE_FREQ);
			step1 = G_PRESCALER * HZ;
			break;
		case RED:
			HZ = 1000/R_TIMER_RES;
			step1 = R_PRESCALER * HZ;
			break;
		case YELLOW:
			HZ = 1000/Y_TIMER_RES;
			step1 = Y_PRESCALER * HZ;
			break;
		default:
			break;
	}	
	step2 = (f_IO / step1) - 1;
	return step2;
}
