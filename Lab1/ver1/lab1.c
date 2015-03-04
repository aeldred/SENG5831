#define ECHO2LCD

#include <pololu/orangutan.h>

#include "led.h"
#include "timer.h"
#include "menu.h"

#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile uint32_t Y_TICKS = 0;
volatile uint32_t G_TICKS = 0;
volatile uint32_t MS_TICKS = 0;

volatile uint16_t RELEASE_RED = 0;

volatile uint32_t R_TOGGLE = 0;
volatile uint32_t Y_TOGGLE = 0;
volatile uint32_t G_TOGGLE = 0;

volatile uint16_t R_TOGGLE_FREQ = 1;
volatile uint16_t Y_TOGGLE_FREQ = 1;
volatile uint16_t G_TOGGLE_FREQ = 1;

int main(void) {

	//print to serial comm window
	char tempBuffer[32];
	int length = 0;

	//initialization
	lcd_init_printf();
	init_LEDs();		//test all the leds
	init_timers();		
	init_menu();

	clear();

	//enable interrupts
	sei();

	while (1) {
		/* 1 Toggle red every 1000MS using for loop */
//		check_red_toggle();

		/* 2 Use software timer ISR to schedule red led toggle */
//		if (RELEASE_RED) {
//			toggle(RED);
//		}
			
		/* check for user input from menu */

		serial_check();
		check_for_new_bytes_received();

	}
}
