#include <pololu/orangutan.h>

/* Andrew Eldred
 * led1: SENG5831 - Week1
 *
 * This program uses pushbuttons to toggle green and red blinking leds
 * Once button is pressed, the led will continuously blink until the 
 * button is pressed again to toggle it off
 *
 * To toggle a green flashing led, press the top button 
 * To toggle a red flashing led, press the bottom button 
 * 
 * The pushbutton must be pressed until the state of the led changes (approx 2 seconds)
 * This program operates in a single thread, so program needs enough time to loop
 * through the blinking logic and catch the pushbutton action the next time it enters
 * button_press function.
 */

int main()
{
  int blink_red = 0;
  int blink_green = 0;
  int red_on = 0;
  int green_on = 0;
  clear();
	

  while(1)
  {
	//handle red led	
	if (blink_red) {
		lcd_goto_xy(0,1);
		print("Red -> ON ");
		if (red_on) {
			red_led(0);
			red_on = 0;
		} else {
			red_led(1);
			red_on = 1;
		}
	} else {
		lcd_goto_xy(0,1);
		print("Red -> OFF");
		red_led(0);
		red_on = 0;
	} 

	//handle green led
	if (blink_green) {
		lcd_goto_xy(0,0);
		print("Green -> ON ");
		if (green_on) {
			green_led(0);
			green_on = 0;
		} else {
			green_led(1);
			green_on = 1;
		}
	} else {
		lcd_goto_xy(0,0);
		print("Green -> OFF");
		green_led(0);
		green_on = 0;
	}
	
  	//identify if top or bottom button is pressed. If no button pressed, returns 0
  	unsigned char button = get_single_debounced_button_press(ANY_BUTTON);

	switch (button) {
		case BOTTOM_BUTTON:
			if (blink_red) {
				blink_red = 0;
			} else {
				blink_red = 1;
			}
			break;
		case MIDDLE_BUTTON:
			break;
		case TOP_BUTTON:
			if (blink_green) {
				blink_green = 0;
			} else {
				blink_green = 1;
			}
			break;
	}
	delay_ms(200);
  }
}
 
