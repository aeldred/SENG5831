#include <pololu/orangutan.h>
#include "led.h"
#include "timer.h"
#include <stdio.h>
#include <pololu/time.h>


extern uint32_t R_TOGGLE;
extern uint32_t Y_TOGGLE;
extern uint32_t G_TOGGLE;

extern uint16_t R_TOGGLE_FREQ;
extern uint16_t Y_TOGGLE_FREQ;
extern uint16_t G_TOGGLE_FREQ;

extern uint32_t Y_TICKS;

extern uint16_t RELEASE_RED;

int R_PERIOD = 10;

long R_PREV_TIME;

void init_LEDs () {
	clear();
	lcd_goto_xy(0,0);
	print("INIT LEDs");

	//clear data direction ports
	R_OUT &= ~(1 << R_PIN);
	G_OUT &= ~(1 << G_PIN);
	Y_OUT &= ~(1 << Y_PIN);
	
	//R_PORT |= (1 << R_PIN);
	R_PORT &= ~(1 << R_PIN);
	G_PORT &= ~(1 << G_PIN);
	Y_PORT &= ~(1 << Y_PIN);
	

	//set data direction as output	
	R_OUT |= (1 << R_PIN);	
	G_OUT |= (1 << G_PIN);	
	Y_OUT |= (1 << Y_PIN);	
	
	//turn on LEDs to check they are working
	//R_PORT &= ~(1 << R_PIN);	
	R_PORT |= (1 << R_PIN);	
	G_PORT |= (1 << G_PIN);
	Y_PORT |= (1 << Y_PIN);

	//wait 1 sec
	int i;
	for (i = 0; i<100; i++) {
		WAIT_10MS;
	}

	//turn LEDs off
	//R_PORT |= (1 << R_PIN);		//turn off signal for red led
	R_PORT &= ~(1 << R_PIN);		//turn off signal for red led
	G_PORT &= ~(1 << G_PIN);	//turn off signal for red led
	Y_PORT &= ~(1 << Y_PIN);	//turn off signal for red led

	//clear toggle counters
	R_TOGGLE = 0;
	Y_TOGGLE = 0;
	G_TOGGLE = 0;

	//pause before continuing program
	WAIT_10MS;
	clear();	
}

/*toggle********************************
 each type of led will be toggled in a specific way

RED -> software timer
YELLOW -> interrupts
GREEN -> PWM
****************************************/
void toggle (led) {
	switch(led) {
		case RED:
			R_PORT ^= (1 << R_PIN);
			R_TOGGLE++;
			RELEASE_RED = 0;
			break;
		case YELLOW:
			Y_PORT ^= (1 << Y_PIN);
			Y_TOGGLE++;
			break;
		case GREEN:
			G_TOGGLE++;
			break;
		default:
			break;
	}
}

void set_toggle(led, ms) {
	//lcd_goto_xy(0,1);
	//print("SET TOGGLE");
	if (ms < 0 ) {
		printf("Cannot toggle negative ms.\n");
		return;
	}

	//adjust for max granularity of 100ms
	if (~((ms%100)==0)) {
		ms = ms - (ms%100);
	}

	if (led==RED || led==ALL) {
		if (ms == 0) {
			R_OUT &= ~(1 << R_PIN);
		} else {
			R_OUT |= (1 << R_PIN);
			R_TOGGLE_FREQ = ms;
		}
	}

	if (led==YELLOW || led==ALL) {
		if (ms == 0) {
			Y_OUT &= ~(1 << Y_PIN);
		} else {
			Y_OUT |= (1 << Y_PIN);
			Y_TOGGLE_FREQ = (ms / Y_TIMER_RES);
			toggle(YELLOW);
		}
	}

	if (led==GREEN || led==ALL) {
		if (ms == 0) {
			G_OUT &= ~(1 << G_PIN);
		} else {
			G_OUT |= (1 << G_PIN);
			if (ms > 4000) { 
				ms = 4000;
			}
			G_TOGGLE_FREQ = (ms / Y_TIMER_RES);
			OCR1A = calculate_OCRA(GREEN);		
		}
	}
}

void check_red_toggle() {
	long currTime;
	currTime = get_ms();

	if ((currTime - R_PREV_TIME) >= (R_TOGGLE_FREQ * R_TIMER_RES)) {
		RELEASE_RED = 1;
		R_PREV_TIME = currTime;
	}
}
