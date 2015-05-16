/** led_pattern.h
**/

#ifndef __LED_PATTERN_H
#define __LED_PATTERN_H


rgb_color * rainbow() {

	static rgb_color this_pattern[7] = {
		(rgb_color){255,0,0},
		(rgb_color){255,137,0},
		(rgb_color){255,255,0},
		(rgb_color){0,255,0},
		(rgb_color){0,0,255},
		(rgb_color){255,0,255},
		(rgb_color){162,0,255}
	};

	return this_pattern;	
}

#endif	//__LED_PATTERN_H
