/**
main.c


**/

#ifndef F_CPU
#define F_CPU				 20000000UL	// 20 MHz
#endif

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "led.h"
#include "led_pattern.h"

//#include <stdio.h>

#define LED_COUNT 18
rgb_color colors[LED_COUNT];

// the number of slices that make up the entire platter
// this is set to 255 to correspond with the number of timing of 8bit timer0
#define SEGMENTS			 0xFF

// PINS for sensor and LED strip
#define SensorInput	PIND3		//ext interrupt on INT1
#define LEDOutput	PIND2


// The timers are configured with the prescaler set to 8, which means every
// 8 clock cycles equals on tick on the counter.	This is a constant to help
// convert timer cycles back to real time.
#define FREQ_SEG			(F_CPU / 8)

// Defines how many ticks a millisecond equals on our clock
#define TICKS_PER_MS		 (FREQ_SEG * .001)

// This is the minimum number of ticks per 1 rotation that will
// be considered a valid period
// The math behind the value is:
// 7200 rpm = 120 rps; 1 rev =~ 8ms; 
// CPU FREQ / 8 (prescaler) * .001 = number of ticks in 1 ms on our setup
// 1ms = 2500 ticks; 1 rev should equal around 20,000 ticks
#define MIN_TICKS			(TICKS_PER_MS * 8)			

volatile int led_color_change = 0;
volatile int sensor_detected = 0;
volatile int segment_cnt = 0;
int period = 0;

// Configure LED Pins, Timers, and Hardware Interrupt
void initHardware()
{
	// disable global interrupts
	cli();

	// timer0 (8bit) acts as a software interrupt that is triggered
	// each time a new segment of the disc is started. When this 
	// happens the led color needs to be changed to the next color
	// for the pattern being drawn
	TCCR0A = 0;
	TCCR0B = 0;	
	// timer0 will operate in CTC mode and should be cleared on
	// match so that the next period per segment can be clocked
	TCCR0A = 0x82;
	// set the prescaler to 8
	TCCR0B = 0x02;
	// enable compare interrupt on OCR0A
	TIMSK0 = 0x02;
	
	// timer1 operates in normal mode and simply provides a value for the 
	// period of the platter's rotation
	TCCR1A = 0;
	// set prescaler clk / 8
	// so every 8 clock cycles equals a tick in TCNT1
	TCCR1B = 0x02;	
	// reset timer value
	TCNT1 = 0;
	// enable overflow interrupt
	TIMSK1 = 0x01;

	// External interrupt for sensor
	// int1 (PIND3), on falling
	// first disable INT1 in EIMSK by clearing its bit so that an 
	// an interrupt can't be triggered on INT1 while register is changed
	EIMSK &= 0xFD;
	EICRA = 0x0C;
	EIFR |= 0x02; 
	EIMSK |= 0x02;

	// set the rotational period to 0
	period = 0;
	
	// enable global interrupts
	sei();

}


/**
 Interrupts
**/

// This interrupt is called on every pulse of the sensor, which represents
// one full rotation of the platter.	It is responsible for timing the
// rotations and setting the compare match value for timer0
ISR(INT1_vect)
{
	// notify main program that a full rotation of the platter has occurred
	sensor_detected = 1;

	// get the period for the platter rotation from timer1 and then reset
	// the timer to get the period for the next rotation
	// AtMega datasheet suggests atomic reads for 16-bit timers because
	// the timer requires two reads, one each for low and high bytes
	unsigned char sreg;
	sreg = SREG;
	cli();
	period = TCNT1;
	TCNT1 = 0;
	SREG = sreg;
	sei();

	// reset the timer controlling the LEDs so that it starts back at
	// position 0 and accurately displays LEDs at exact moment the slot
	// is in position
	TCNT0 = 0;
	
	// if the period is too small, then the system is probably just starting
	// up or it's a ghost signal, so don't do anything with the 
	// LEDs until the next rotation is clocked
	if (period < MIN_TICKS) {
		return;
	}

	// set the timer0 compare match value so that the correct color is drawn
	// at the exact moment the slot is at that segment
	// this corresponds to the period per segment
	OCR0A = period / SEGMENTS;
	
}

// This interrupt is called when it is time to draw the next segment color
// This is triggered by the value set in OCR0A when the INT1 interrupt is 
// triggered.
ISR(TIMER0_COMPA_vect) {
	led_color_change = 1;
}

// If the platter spin time overflows timer1, this is called
ISR(TIMER1_OVF_vect) {
}

int main() {
	// get the pattern that will be displayed 
	// in future updates this should be modifiable via some sort of user input
	rgb_color * pattern_colors = rainbow();		
	int num_colors = sizeof(pattern_colors)/sizeof(pattern_colors[0]);
	int seg_per_color = SEGMENTS / num_colors;
	volatile int curr_color = 0;
	
	while(1) {
		if (sensor_detected) {
			//reset the interrupt flag
			sensor_detected = 0;
		}

		if (led_color_change) {
			// the specific pattern is composed of an array of rgb_color structs,
			// for this implementation, we will assume an even distribution of 
			// colors across all segments
			// so need to determine which color should be displayed right now
			// based on the value of segment_cnt
			if ((segment_cnt % seg_per_color) == 0) {		
				curr_color = curr_color + 1;

				//might have uneven number of segments in relation to number
				// of colors, so make sure we haven't gone past end of patter_color
				// array. For now, just populate remaining segments with the final color
				if (curr_color >= num_colors) {
					curr_color = num_colors - 1;
				}
			}

			// LEDs are individually addressable so need next color for each LED
			// each position in the colors array represents 1 LED
			int i;
			for(i = 0; i < LED_COUNT; i++) {
				colors[i] = *(pattern_colors + curr_color);
			}

			//increment to the next segment and wrap the value if > SEGMENTS 
			segment_cnt = (segment_cnt + 1);
			if (segment_cnt > SEGMENTS) {
				segment_cnt = 0;
				curr_color = 0;
			}

			// reset the interrupt flag
			led_color_change = 0;
		}
	}
}
