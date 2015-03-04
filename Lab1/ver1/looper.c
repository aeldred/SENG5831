/****************************
* Looper - determine timing of loops to
* get value for LOOP_CNT_10MS
****************************/
#include <inttypes.h>
#include <stdio.h>
#include <pololu/time.h>
#include <pololu/Orangutan.h>
volatile uint64_t i;

int main() {
	lcd_init_printf();
		clear();
	while (1) {

		volatile long empty_loop_start = get_ms(); 
		for (i=0;i<2123;i++) {
			volatile int i = 0; i = i + 1;
		}
		volatile long empty_loop_end = get_ms();

		volatile long empty_loop_time = (empty_loop_end - empty_loop_start);

		lcd_goto_xy(0,0);
		printf("MS: %12lu", empty_loop_time);
		delay_ms(1000);
		
	}
}
