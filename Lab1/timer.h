/**********************
* Timer header file
**********************/
#ifndef __TIMER_H
#define __TIMER_H

#include <inttypes.h>

//number of loops to approx 10ms
#define LOOP_CNT_10MS 2123	

//loop counter
volatile uint64_t _ii;

//this is our loop that will take 10MS
#define WAIT_10MS 	{ for (_ii=0; _ii < (LOOP_CNT_10MS); _ii++) { volatile int i = 0; i = i + 1;} }

#define R_TIMER_RES	1
#define Y_TIMER_RES	100
#define G_TIMER_RES	100

void init_timers();
int calculate_OCRA(int);

#endif
