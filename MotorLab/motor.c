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

#include <pololu/orangutan.h>
#include <stdio.h>

int direction = 1;
int INCREMENT = 10;
int speed_m1 = 0;
int speed_m2 = 0;

void set_motor_speed(unsigned char button) {
	switch(button) {
		case TOP_BUTTON:
			speed_m1 += INCREMENT;	
			set_motors(speed_m1*direction, speed_m2*direction);
			break;
		case MIDDLE_BUTTON:
			if (direction > 0) {
				direction = -1;
			} else if (direction < 0) {
				direction = 1;
			} else {
				direction = 1;
			}

			if (speed_m1 > 0) {
				int curr_speed_m1 = speed_m1;
				while (speed_m1 > 0) {
					speed_m1 -= INCREMENT;
					set_motors(speed_m1*direction, speed_m2*direction);
					delay_ms(500);
				}
				while (speed_m1 < curr_speed_m1) {
					speed_m1 += INCREMENT;
					set_motors(speed_m1*direction, speed_m2*direction);
					delay_ms(500);
				}	
			} else {
				set_motors(speed_m1*direction, speed_m2*direction);
			}	
			break;
		case BOTTOM_BUTTON:
			speed_m1 -= INCREMENT;
			if (speed_m1 < 0) {
				speed_m1 = 0;
			}
			set_motors(speed_m1*direction, speed_m2*direction);
			break;
	}
	
}

int main()
{
	lcd_init_printf();
	// Initialize the encoders and specify the four input pins.
	encoders_init(IO_A0, IO_A1, IO_C4, IO_C5);

	set_motors(0,0);
	while(1) {
    	// Read the counts for motor 1 and print to LCD.
    	lcd_goto_xy(0,0);
		printf("%-10d",encoders_get_counts_m1());
    	//print_long(encoders_get_counts_m1());
    	print(" ");

    	// Print encoder errors, if there are any.
    	if(encoders_check_error_m1()) {
      		lcd_goto_xy(0,1);
      		print("Error 1");
 		}

		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		if (button) {
			set_motor_speed(button);
			if (speed_m1 == 0) {
				delay_ms(100);
				encoders_get_counts_and_reset_m1();
			}
		}


		lcd_goto_xy(0,1);
		printf("SPEED:%-3d",speed_m1);

		lcd_goto_xy(10,1);
		if (direction < 0) {
			print("BACK");
		} else if (direction > 0) {
			print("FWD ");
		} 

    	delay_ms(50);
	}
}
