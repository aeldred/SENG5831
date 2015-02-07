#include <avr/io.h>
#include <pololu/orangutan.h>
#include <stdio.h>  

/* 
   This program will blink one led when the top button
   is pressed and a second led when the bottom button is
   pressed (I'm using the built in green led as the second led). 
   The leds will blink independently.

   The rate of blink can be controlled via the USB_COMM
   using the + and - buttons to increase and decrease the rate
   respectively (for symmetry in controls, 
   the = button will also increase the blink rate).

   The blink rate will only be adjusted for leds that are
   currently blinking when the + and - buttons are pressed.
   If turned off and then back on, the blink rate will resume
   with the most recent blink rate before it was turned off.
*/

//serial buffer vars
char receive_buffer[32];
unsigned char receive_buffer_pos = 0;

//blinking functionality on/off flags
unsigned int top_button_on = 0;
unsigned int bottom_button_on = 0;

//last blink time for each button
unsigned int tbTime = 0;
unsigned int bbTime = 0;

//rate of blinking for each button
unsigned int top_blink_rate = 2000;
unsigned int bottom_blink_rate = 1000;

unsigned int INCREMENT = 100;


void wait_for_send_to_finish() {
	while(!serial_send_buffer_empty(USB_COMM)) {
		serial_check();
	}
}

void process_recvd_byte(char byte) {
	switch(byte) {
		case '+':
			if (top_button_on) {
				top_blink_rate += INCREMENT;
			}
			if (bottom_button_on) {
				bottom_blink_rate += INCREMENT;
			}
			break;
		case '=':
			if (top_button_on) {
				if (top_blink_rate <= 0) {
					top_blink_rate = INCREMENT;
				} else {
					top_blink_rate += INCREMENT;
				}

			}
			if (bottom_button_on) {
				if (bottom_blink_rate <= 0) {
					bottom_blink_rate = INCREMENT;
				} else {
					bottom_blink_rate += INCREMENT;
				}
			}
			break;
		case '-':
			if (top_button_on) {
				if (top_blink_rate <= 0) {
					top_blink_rate = 0;
				} else {
					top_blink_rate -= INCREMENT;
				}
			}
			if (bottom_button_on) {
				if (bottom_blink_rate <= 0) {
					bottom_blink_rate = 0;
				} else {
					bottom_blink_rate -= INCREMENT;
				}
			}
			break;
		default:
			wait_for_send_to_finish();
			break;
	}

	if (top_button_on) {
		lcd_goto_xy(6,0);
		printf("%8d",top_blink_rate);
	}
	if (bottom_button_on) {
		lcd_goto_xy(6,1);
		printf("%8d",bottom_blink_rate);
	}
}

void check_for_new_recvd_bytes() {
	while (serial_get_received_bytes(USB_COMM) != receive_buffer_pos) {
		process_recvd_byte(receive_buffer[receive_buffer_pos]);

		if (receive_buffer_pos == sizeof(receive_buffer) - 1) {
			receive_buffer_pos = 0;
		} else {
			receive_buffer_pos++;
		}
	}
}

void blink_light(int delay1, int delay2) {
	unsigned int currTime = get_ms();

	if (top_button_on) {
		if (currTime - tbTime >= delay1) {
			PORTD ^= 0x04;
			tbTime = currTime;	
		}
	}
	if (bottom_button_on) {
		if (currTime - bbTime >= delay2) {
			PORTC ^= 0x10;
			bbTime = currTime;	
		}
	}
}

void toggle_blink_func(unsigned char button) {
	switch(button) {
		case TOP_BUTTON:
			if (top_button_on) {
				top_button_on = 0;
				PORTD &= ~(0x04);
				lcd_goto_xy(0,0);
				print("led1:          ");
			} else {
				top_button_on = 1;
				lcd_goto_xy(0,0);
				print("LED1:      ");
				lcd_goto_xy(6,0);
				printf("%8d",top_blink_rate);
			}
			break;
		case MIDDLE_BUTTON:
			break;
		case BOTTOM_BUTTON:
			if (bottom_button_on) {
				bottom_button_on = 0;
				PORTC &= ~(0x10);
				lcd_goto_xy(0,1);
				print("led2:         ");
			} else {
				bottom_button_on = 1;
				lcd_goto_xy(0,1);
				print("LED2:      ");
				lcd_goto_xy(6,1);
				printf("%8d",bottom_blink_rate);
			}
			break;
	}
}

int main() {
	lcd_init_printf();
	clear();
	lcd_goto_xy(0,0);
	print("led1: ");
	lcd_goto_xy(0,1);
	print("led2: ");

	serial_set_baud_rate(USB_COMM, 9600);
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

	//D1 = 0x02  -- red built-in 
	//D2 = 0x04
	//C4 = 0x10 -- green built-in  

	//set for output
	DDRD = 0x04;
	DDRC = 0x10;
	
	while(1) {
		serial_check();
		check_for_new_recvd_bytes();

		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		if (button) {
			toggle_blink_func(button);
		}

		blink_light(top_blink_rate, bottom_blink_rate);
	}
}

