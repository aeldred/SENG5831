/*
 * wheel-encoders1: example for the Orangutan LV, SV, SVP, or X2.  It
 * will work on the Baby Orangutan, though the LCD functions will have
 * no effect.
 * Note that the Orangutan SVP has two dedicated ports for quadrature
 * encoders, so using the PololuWheelEncoders library is not necessary
 * on the Orangutan SVP (the encoders are handled by an auxiliary MCU).
 *
 * This example measures the outputs of two encoders, one connected to
 * ports PC2 and PC3, and another connected to ports PC4 and PC5.
 * Those pins are not easily accessible on the Orangutan SVP or
 * Orangutan X2, so you will probably want to change the pin
 * assignments if you are using one of those devices.
 *
 * http://www.pololu.com/docs/0J20
 * http://www.pololu.com
 * http://forum.pololu.com
 */

#include "menu.h"
#include "motor.h"
#include <pololu/orangutan.h>
#include <stdio.h>

int main()
{
	lcd_init_printf();
	clear();

	//disable interrupts before initializing
	cli();

	//initializations
	init_prog();
	init_menu();
	init_motor();
	init_timers();
	init_encoders();


	//enable interrupts
	sei();


	while(1) {
		//read the encoder value and output to the lcd
		int encoder = get_encoder_val();
		lcd_goto_xy(0,0);
		printf("ENC: %-16d",encoder);

		//if it's time to calculate the speed/position run the pid controller
		if (is_calc_released()) {			
			pid_controller();

		}


		/* check for user input from menu */
		// NOTE: My menu system is very unforgiving and has a bug in it
		// that causes the counter to be off by one when it reaches end of the buffer. 

		// BUG: A command is entered but nothing happens 
		//   Hit enter a second time and it will run. The buffer count gets off by 1 somehow

		// BUG: The end of the command is cut off (i.e. r 1000  is read in as r 100)
		//    Re-enter the command at the next prompt. Again caused by buffer count off by 1
		serial_check();
		check_for_new_bytes_received();
	}
}
