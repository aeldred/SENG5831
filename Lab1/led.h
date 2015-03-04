/***********************************
* LED header file
***********************************/
#ifndef __LED_H
#define __LED_H

#include <inttypes.h>

//A2 port for extern red led
//#define R_PORT	PORTA		//port location for red led
//#define R_OUT		DDRA		//register
//#define R_PIN		PINA2
//port for onboard red led 
//#define R_PORT	PORTD
//#define R_OUT		DDRD
//#define R_PIN		PIND1 	

#define ALL		3

//I only have 2 leds, so am using on-board green led
#define RED 		0
#define R_PORT		PORTC
#define R_OUT		DDRC
#define R_PIN		PINC4 	

#define GREEN 		1
#define G_PORT	PORTD		//port location for green led
#define G_OUT	DDRD		//register
#define G_PIN	PIND5		//bit to turn on

#define YELLOW 		2
#define Y_PORT	PORTA		//port location for green led
#define Y_OUT	DDRA		//register
#define Y_PIN	PINA0		//bit to turn on


void toggle(int);
void set_toggle(int, int);
void init_LEDs();		
void reset_LEDs();
void check_red_toggle();

#endif
