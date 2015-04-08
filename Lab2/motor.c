#include "motor.h"
#include "menu.h"

#include <pololu/orangutan.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//testing
long prev_test_time = 0;
int prev_test_cnt = 0;
int total_test_cnt = 0;
int total_tests = 0;

//globals
long f_IO = 20000000;
int PRESCALER = 1024;
double calcPeriod = .02;			//sampling rate
//double calcPeriod = .1;			//sampling rate

double gErr = 0;
int gTorque = 0;

int TOP_SPEED = 255;
int speed = 0;				

int speedExec = 0;
int positionExec = 0;	
int trajectoryExec = 0;

int traj_part1 = 0;
int traj_part2 = 0;
int traj_part3 = 0;
int traj_done = 0;
int t_part1_cnts = 0;
int t_part2_cnts = 0;
int t_part3_cnts = 0;

char tempBuffer[128];

//How close can I get to my desired position
//if the position isn't changing after 10 tries
//assume we've gotten as close as we can
double margin_err = 10;
int calc_pos_zero = 0;

unsigned long prevTime;
double prev_err_spd;
double prev_err_pos;
double integral_spd;
double integral_pos;

/* best values for speed
double kp = 0.95;
double ki = 1.400;
double kd = 0.0650;
*/

/* best values for pos */
/* attempt 1
double kp = 2.30;
double ki = 0.060;
double kd = 0.0081;*/

// values for trajectory
double kp = 1.15;
double ki = 0.000;
double kd = 0.0000;

double gain_interval = .05;
double i_gain_interval = .01;
double d_gain_interval = .0001;

char * trajectory = "F90 H.5 B360 H.5 F5";

volatile char m2a_val;
volatile char m2b_val;
volatile char m2a_last_val = -1;
volatile char m2b_last_val = -1;
volatile int m2_count;
volatile int m2_err = 0;
volatile int isCalcReleased = 0;

volatile int prev_m2_spd_cnt = 0;
volatile int prev_m2_pos_cnt = 0;

// known values for encoder
double PULSES_PER_REV = 2249.0;

//this is an average of 12 encoder counts taken at time=1sec
double PULSES_PER_SEC = 7765.0;

//the expected pos at each sampling period
double MAX_AT_SAMPLING;

// the total distance we've travelled towards our destination
int total_calc_pos = 0;
int total_ref_pos = 0;
double total_pos = 0;
double prev_pos_pid = 0;

// this is where we expect to actually be when the entire trajectory is complete
int total_exp_pos = 0;

double calc_pos;
double calc_speed;

double ref_speed = 0;
double ref_pos = 0;
int ref_dir = 1;
double prev_ref_pos = 0;

long prev_time = 0;

int loggingOn = 0;

// timer for calculating torque for motor
// the ISR just sets a flag that main then looks for
ISR(TIMER1_COMPA_vect) {
	set_calc_complete(0);
}

// interrupt for encoder counts
ISR(PCINT1_vect) {
    //check for high/low signal
    m2a_val = ((PINA & (1<<PINA2)) > 0);
    m2b_val = ((PINA & (1<<PINA3)) > 0);

	//if (m2a_last_val == -1 && m2b_last_val == -1) {
	//	m2_count = 0;
	//} else { 
		//check how input signal has changed
		if (m2a_val ^ m2b_last_val) {
			m2_count += 1;
		}
		if (m2b_val ^ m2a_last_val) {
			m2_count -= 1;
		}
	//}
	
	//check for error, though not doing anything with it
	if (m2a_val != m2a_last_val && m2b_val != m2b_last_val) {
		m2_err = 1;
	}

	// store previous value
	m2a_last_val = m2a_val;
	m2b_last_val = m2b_val;

}


void init_prog() {

	// I have an issue somewhere in my code that sporadically causes
	// my board to shut off - likely an overflow of some type

	// The AtMega datasheet Note at bottom of page 56 recommends clearing the
	// watchdog flags on initialization in case a pointer mistakenly
	// accesses the flag. This will cause behavior simliar to what I am
	// seeing, so added this in as a possible fix

	// Seemed like it happened less frequently after adding this, but still 
	// occurred occasionally...
	/* Clear WDRF in MCUSR */
	MCUSR &= ~(1<<WDRF);
	/* Clear WDE bit*/
	WDTCSR &= ~(1<<WDE);
}

// this initializes the interrupt for calculating the next input to the motor
void init_timers() {
	
	//---SPEED CALC TIMER--------------------
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1C = 0;
	TCNT1 = 0;

	TCCR1A |= 0x43;		
	TCCR1B |= 0x1D;			//1024 prescaler
	TCCR1C |= 0x00;
	TIMSK1 |= (1 << OCIE1A);
	OCR1A = calculate_OCRA();	//max TOP 0xFFFF;
	OCR1B = 1;

	//this is the expected distance travelled based on the sampling rate 
	MAX_AT_SAMPLING = calcPeriod * PULSES_PER_SEC;
}

int calculate_OCRA () {
	double step1;
	long step2;
	step1 = calcPeriod * f_IO;
	step2 = (long)((step1/PRESCALER) - 1);

	if (step2 > 0xFFFF) {
		step2 = 0xFFFF;
	}
	return step2;
}

// this initializes the interrupt for the encoder count
void init_encoders() {
	DDRA &= ~(1<<PCINT10);
	PORTA |= (1<<PCINT10);
	DDRA &= ~(1<<PCINT11);
	PORTA |= (1<<PCINT11);

	//enable pin change interrupt	
	PCICR |= (1 << PCIE1);

	//enable interrupt for pin 2 and 3
	PCMSK1 |= (1 << PCINT10);
	PCMSK1 |= (1 << PCINT11);

}

// this initializes pwm for motor control
void init_motor() {
	//Timer2 (motor 2 uses Timer2)
	//initialize timers 
	TCCR2A = 0x23;

	//use sys clock = pwm frequency of 10kHz
	TCCR2B = 0x02;
	
	//initialize to almost braking //this is setting the initial duty cycle
	OCR2A = 0;
	OCR2B = 0;

	//set drive low
	PORTD &= ~(1<<PIND6);		//pwm
	PORTC &= ~(1<<PINC6);		//init direction forward
	
	//set pins to output(1)
	DDRD |= (1<<PIND6);
	DDRC |= (1<<PINC6);

}

// speed (torque) is received as the value to be assigned to pwm
void set_speed(int torque) {
	unsigned char reverse = 0;

	// set a global value - really just included this so I can access it from menu.c
	set_torque(torque);

	speed = torque;
	
	// if the speed is negative, use absolute val and set reverse flag
	if (speed < 0) {
		speed = abs(speed);
		reverse = 1;
	}

	//speed can't be > than TOP_SPEED = 255 
	if (speed > TOP_SPEED) {
		speed = TOP_SPEED;
	}

	OCR2B = speed;

	//set direction of motor on PC6
	//R: PC6 high
	//F: PC6 low
	if (reverse) {
		PORTC |= (1<<PINC6);
	} else {
		PORTC &= ~(1<<PINC6);
	}
}

// calculate the current speed and position of the motor
// the calc'd values are stored in global vars calc_speed and calc_pos respectively
void pid_controller() {
	/*-------PID for SPEED------------*/
	if (speedExec) {
	
		calculate_speed();

		// Notes: experimenting showed that for calc speed, the val from the PID equation
		// had to be added to the current calc_speed.
		// the PID result could be pos or neg depending on under/overshoot

		// compute_t = PID equation implementation
		int torque = round(compute_t(ref_speed, calc_speed, 1) + calc_speed);

		// set the speed of the motor
		//get the pwm value as a percentage of the top speed (255)
		//PULSES_PER_SEC is the number of pulses measured at top speed ()
		int adj_torque = round((torque/PULSES_PER_SEC) * TOP_SPEED);	

		// send the torque value to the motor
		set_speed(adj_torque);

		// print useful info
		if (ref_speed != 0 && loggingOn > 0) {
			sprintf(tempBuffer, "REF: %.2f   CALC: %.2f    TORQUE: %d\n",ref_speed, calc_speed, torque);
			int tempLen = strlen(tempBuffer);
			print_usb(tempBuffer, tempLen);
		}
	
	} 

	/*--------PID for POSITION-------------*/
	// this handles both positions and each segment of the trajectory
	else if (positionExec || (trajectoryExec && ref_pos != 0)) {
		//get current position of motor
		calculate_pos();

		total_pos += calc_pos;

		// PID equation is using the ref/calc values for the current period
		double adj_ref = ref_pos - total_pos;
		if (adj_ref > MAX_AT_SAMPLING) {
			adj_ref = MAX_AT_SAMPLING;
		} else if (adj_ref < -MAX_AT_SAMPLING) {
			adj_ref = -MAX_AT_SAMPLING;
		}

		// played around with whether to use the current period's calc/ref values
		// or the cal/ref values for the entire position supplied			
		// it seemed off to consider the entire position as the ref when the motor
		// isn't capable of going that distance in a single period...

		//double pid_pos = compute_t(ref_pos, total_pos, 0);

		// get PID value for the cal/ref position for the current period
		double pid_pos = compute_t(adj_ref, calc_pos, 0);

		// do some fancy rounding to get the pwm value
		int torque = round((pid_pos/(double)MAX_AT_SAMPLING)* (double)TOP_SPEED);

		// we will know that we've arrived at our destination if the torque changes direction (we've gone too far)
		// or the torque is 0 (we've arrived at our point)
		if (((ref_pos - total_pos) <= 0 && ref_dir > 0) || ((ref_pos - total_pos) >= 0 && ref_dir < 0) || (calc_pos == 0 && abs(torque) > 0)) {
			// check if the motor might be stalled
			if (calc_pos == 0 && abs(torque) > 0) {
				calc_pos_zero++;
			} else {
				// just set this as a flag to trigger end of movement
				calc_pos_zero = margin_err;
			}
			
			// the motor is at desired position (or as close as it can get)	
			if (calc_pos_zero == margin_err) {
				//logging	
				double overshoot = total_pos - ref_pos;
				double spd = calc_pos/calcPeriod;

				sprintf(tempBuffer, "POS: %.2f   SPD: %.2f   REF: %.2f  ERR: %.2f  \n",calc_pos, spd, adj_ref,gErr);
				int tempLen = strlen(tempBuffer);
				print_usb(tempBuffer, tempLen);
		
				sprintf(tempBuffer, "TOT: %.2f   REF: %.2f OVERSHOOT: %.2f\n",total_pos, ref_pos, overshoot);
				tempLen = strlen(tempBuffer);
				print_usb(tempBuffer, tempLen);

				//house cleaning - stop executing the position code and clean up flags
				traj_done = 1;
				ref_pos = 0;
				total_pos = 0;
				set_exec_position(0);
				set_speed(0);
				calc_pos_zero = 0;
			} else {
				// haven't gotten to posistion yet, so set the motor to PID value
				// this is here in case the motor seems to be stalled but we're not at our position yet
				// there's a limit on how many times we can get to this block (margin_err)
				set_speed(torque);
			}
		} else {
			// keep going
			calc_pos_zero = 0;
			set_speed(torque);
		}
	
		// print info worth seeing 
		if (positionExec && loggingOn > 0) {
			double spd = calc_pos/calcPeriod;

			sprintf(tempBuffer, "POS: %.2f   SPD: %.2f   REF: %.2f  ERR: %.2f  TOR: %d  \n",calc_pos, spd, adj_ref,gErr,torque);
			//sprintf(tempBuffer, "POS: %.2f   SPD: %.2f   REF: %.2f  ERR: %.2f   \n",calc_pos, spd, adj_ref,gErr);
			//sprintf(tempBuffer, "POS: %.2f   SPD: %.2f   REF: %.2f  ERR: %.2f   \n",total_pos, spd, ref_pos,gErr);
			int tempLen = strlen(tempBuffer);
			print_usb(tempBuffer, tempLen);
		}

	}

	// this will handle the trajectory interpolator
	// and sets up the program to head towards the next position in the trajectory
	else if (trajectoryExec && ref_pos == 0) {
		// determine what values to feed to PID equation
		int rv = interpolator();
		trajectoryExec = rv;
	}

	// done with this round of calculations
	set_calc_complete(1);
}

int interpolator() {
	// trajectory in degrees is: F90 H.5 B360 H.5 F5 
	// and have already been parsed into counts
	// 360 deg = 2249 counts

	// the positions we're trying to get to are
	// t_part1_cnts, t_part2_cnts, t_part3_cnts

	// the program will run each trajectory part in order
	// once done, the next ref point is set and flags are adjusted
	// so that the position pid will run
	if (!traj_part1) {
		set_ref_pos(t_part1_cnts);
		trajectoryExec = 1;

		if (traj_done) {
			delay_ms(500);
			traj_part1 = 1;
			traj_done = 0;
			set_ref_pos(t_part2_cnts);
		}
		return 1;
	}
	else if (!traj_part2) {
		
		if (traj_done) {
			delay_ms(500);
			traj_part2 = 1;
			traj_done = 0;
			set_ref_pos(t_part3_cnts);
		}
		return 1;
	}
	else if (!traj_part3) {

		if (traj_done) {
			traj_part3 = 1;
			ref_pos = 0;
		}
		return 1;
	} 
	else {
		// house cleaning
		set_exec_trajectory(0);
		set_exec_position(0);
		return 0;
	}

}

double compute_t(double ref, double calc, int spd_flg) {
	double integral;
	double prev_err;

	double err = ref - calc;
	gErr = err;

	// this could be done for both speed and position in same block of code
	// without the flag, but I didn't want to accidentally overwrite global values
	// that I've hacked together for speed with those for position

	// all this really does is assign the integral value
	if (spd_flg) {
		integral_spd += (err * calcPeriod);
		integral = integral_spd;
		
		prev_err = prev_err_spd;
		prev_err_spd = err;
	} else {
		integral_pos += (err * calcPeriod);
		integral = integral_pos;
		
		prev_err = prev_err_pos;
		prev_err_pos = err;
	}

	double derivative = (err - prev_err) / calcPeriod;

	double p = (kp * err);
	double i = (ki * integral);
	double d = (kd * derivative);
	
	double t = p + i + d;

	return t;
}

// this calculates the  speed during each period
// in counts/sec
void calculate_speed() {
	int curr_m2_cnt = get_encoder_val();

	//calculate change in position of motor shaft
	int m2_pulses = curr_m2_cnt - prev_m2_spd_cnt;

	//calculate speed
	calc_speed = m2_pulses/calcPeriod;

	prev_m2_spd_cnt = curr_m2_cnt;
}

// this calculates the change in position for each
// period
void calculate_pos() {
	int curr_m2_cnt = get_encoder_val();

	//calc change in position of motor shaft
	int m2_pulses = curr_m2_cnt - prev_m2_pos_cnt;

	//lcd_goto_xy(1,1);
	//printf("CNT: %d",m2_pulses);
	
	//set position this period
	calc_pos = m2_pulses;
	
	prev_m2_pos_cnt = curr_m2_cnt;

}

void parse_trajectory(int deg1, int deg2, int deg3) {
	// instead of dealing with rounding, I'm passing in the "degrees" to rotate
	// as counts (i.e. 562, -2249, 31)
	//t_part1_cnts = round((double)(deg1/360) * PULSES_PER_REV);
	//t_part2_cnts = round((double)(deg2/360) * PULSES_PER_REV);
	//t_part3_cnts = round((double)(deg3/360) * PULSES_PER_REV);	

	t_part1_cnts = deg1;
	t_part2_cnts = deg2;
	t_part3_cnts = deg3;

	total_exp_pos = abs(t_part1_cnts) + abs(t_part2_cnts) + abs(t_part3_cnts);
	
	//initialize flags so traj can be rerun by calling parse_trajectory
	prev_ref_pos = 0;
	total_calc_pos = 0;
	total_ref_pos = 0;
	traj_part1 = 0;
	traj_part2 = 0;
	traj_part3 = 0;
	traj_done = 0;
	ref_pos = 0;
	calc_pos_zero = 0;
	
	set_exec_trajectory(1);
	set_exec_position(0);
	set_exec_speed(0);
}

// these are all my helper functions that allow access to vars from menu class
double get_calc_pos() {
	return calc_pos;
}

double get_calc_speed() {
	return calc_speed;
}

int is_calc_released() {
	return isCalcReleased;
}

void set_calc_complete(int complete) {
	isCalcReleased = !complete;			
}

void set_ref_speed(int s) {
	// this is the speed set from the menu
	ref_speed = (double)s;

	// house cleaning hack
	set_exec_position(0);
	set_exec_speed(1);
	set_exec_trajectory(0);
}

void set_ref_pos(int p) {
	ref_pos = (double)p;

	if (ref_pos < 0) {
		ref_dir = -1;
	} else {
		ref_dir = 1;
	}

	total_pos = 0;
	calc_pos_zero = 0;

	// initialize the previous encoder count for first time through
	prev_m2_pos_cnt = get_encoder_val();
	set_exec_position(1);
	set_exec_speed(0);
	set_exec_trajectory(0);
}

double get_ref_speed() {
	return ref_speed;
}
double get_ref_pos() {
	return ref_pos;
}

double get_kp() {
	return kp;
}
double get_ki() {
	return ki;
}
double get_kd() {
	return kd;
}

void set_kp(double val) {
	kp = val;
}
void set_ki(double val) {
	ki = val;
}
void set_kd(double val) {
	kd = val;
}

void change_kp(int dir) {
	if (dir < 0) {
		kp -= gain_interval;
		if (kp < 0) {
			kp = 0;
		}
	} else if (dir > 0) {
		kp += gain_interval;
	}
}
void change_ki(int dir) {
	if (dir < 0) {
		ki -= i_gain_interval;
		if (ki < 0) {
			ki = 0;
		}
	} else if (dir > 0) {
		ki += i_gain_interval;
	}
}
void change_kd(int dir) {
	if (dir < 0) {
		kd -= d_gain_interval;
		if (kd < 0) {
			kd = 0;
		}
	} else if (dir > 0) {
		kd += d_gain_interval;
	}
}
int get_torque() {
	return gTorque;
}
void set_torque(int val) {
	gTorque = val;
}

char * get_trajectory() {
	return trajectory;
}
void set_trajectory(char * traj) {
	strcpy(traj,trajectory);
}

void turn_log_on() {
	loggingOn = 1;
}

void turn_log_off() {
	loggingOn = 0;
}

void set_exec_speed(int flag) {
	speedExec = flag;
}

void set_exec_position(int flag) {
	positionExec = flag;
}

void set_exec_trajectory(int flag) {
	trajectoryExec = flag;

}
	
int check_enc_error() {
	return m2_err;
}

int get_encoder_m2a() {
	return m2a_val;
}

int get_encoder_m2b() {
	return m2b_val;
}

int get_encoder_val() {
	return m2_count;
}
