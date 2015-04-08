/*******************************************
*
* Header file for menu stuff.
*
*******************************************/
#ifndef __MOTOR_H
#define __MOTOR_H

void init_prog();
void init_encoders();
void init_motor();
void init_timers();

void set_speed(int);
int get_encoder_val();
int get_encoder_m2a();
int get_encoder_m2b();

int is_calc_released();
void set_calc_complete(int);

void pid_controller();
void calculate_speed();
void calculate_pos();
double compute_t(double,double,int);
int calculate_OCRA();


//getters and setters
double get_calc_speed();
double get_calc_pos();
void set_ref_speed(int);
void set_ref_pos(int);
double get_ref_speed();
double get_ref_pos();
double get_kp();
double get_ki();
double get_kd();
void set_kp(double);
void set_ki(double);
void set_kd(double);
void change_kp(int);
void change_ki(int);
void change_kd(int);
int get_torque();
char * get_trajectory();
void set_torque(int);
void set_trajectory(char*);
void turn_log_on();
void turn_log_off();
void set_exec_trajectory(int);
void set_exec_speed(int);
void set_exec_position(int);
void parse_trajectory(int,int,int);
int interpolator();
#endif //__MOTOR_H
