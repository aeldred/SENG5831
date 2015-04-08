#include "menu.h"
#include "motor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pololu/orangutan.h>  

// GLOBALS


// local "global" data structures
char receiveBuffer[32];
	//this buffer contains what needs to be processed
	char menuBuffer[32];
unsigned char receive_buffer_pos = 0;
char send_buffer[32];
int tot_bytes_read = 0;

char menu2[40] = "Enter Option: ";
char * menuln1 = "\r\nMenu Options\r\nL/l Start/stop logging Pr, Pm, and T\r\nv   View current settings\r\n";
char * menuln2 = "r   Set reference position in counts\r\ns   Set reference speed in counts/sec\r\n";
char * menuln3 = "P/p Increase/Decrease Kp by gain interval\r\nD/d Increase/Decrease Kd by gain interval\r\n";
char * menuln4 = "I/i Increase/Decrase Ki by gain interval\r\nT/t   Execute trajectory\r\n";
char * menuln6 = "Enter Option: ";


// A generic function for whenever you want to print to your serial comm window.
// Provide a string and the length of that string. My serial comm likes "\r\n" at 
// the end of each string (be sure to include in length) for proper linefeed.
void print_usb( char *buffer, int n ) {
	lcd_goto_xy(0,0);
	serial_send( USB_COMM, buffer, n );
	wait_for_sending_to_finish();
}	
		
//------------------------------------------------------------------------------------------
// Initialize serial communication through USB and print menu options
// This immediately readies the board for serial comm
void init_menu() {
	
	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	print_usb( "\r\nUSB Serial Initialized\r\n", 26);
	
	print_usb( menuln1,strlen(menuln1));
	print_usb( menuln2,strlen(menuln2));
	print_usb( menuln3,strlen(menuln3));
	print_usb( menuln4,strlen(menuln4));
	print_usb( menuln6,strlen(menuln6));
	
	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receiveBuffer, sizeof(receiveBuffer));
}

//------------------------------------------------------------------------------------------
// process_received_byte: Parses a menu command (series of keystrokes) that 
// has been received on USB_COMM and processes it accordingly.
// The menu command is buffered in check_for_new_bytes_received (which calls this function).
void process_received_string(const char* buffer)
{
	// Used to pass to USB_COMM for serial communication
	//int length;
	char tempBuffer[100];
	
	// parse and echo back to serial comm window (and optionally the LCD)
	char uOption;
	int uValue;
	//print_usb((char*)buffer,strlen(buffer));
	sscanf(buffer, "%1c %d", &uOption,&uValue);

	
	// Check valid command and implement
	switch (uOption) {
		case 'L':		
			//start logging - print out Pr, Pm, T
			turn_log_on();
			print_usb("Logging On\r\n",12);
			break;
		case 'l':		
			//stop logging
			turn_log_off();
			print_usb("Logging Off\r\n",13);
			break;
		case 'V':		
		case 'v':
			//view current vals for Kd, Kp, Ki?, Vm, Pr, Pm, T
			sprintf( tempBuffer, "\r\nVm: %d   Pr: %.1f   Pm: %.1f\r\nJ: %s\r\nKp: %.2f   Ki: %.4f   Kd: %.4f\r\n", get_torque(),get_ref_speed(),get_calc_speed(),get_trajectory(),get_kp(),get_ki(),get_kd());
			int length = strlen(tempBuffer);
			print_usb( tempBuffer, length );
			break;
		case 'R':		
		case 'r':
			//set the reference position
			set_ref_pos((double)uValue);
			break;
		case 'S':	
		case 's':
			//set the reference speed 
			//lcd_goto_xy(1,1);
			//printf("NR: %3s",uValue);
			//temp = strtol(uValue,NULL,10);
			//temp = (double)uValue;
			//lcd_goto_xy(1,1);
			//printf("N: %d",uValue);
			set_ref_speed((double)uValue);
			break;
		case 'P':		
			//increase Kp by gain_interval 
			change_kp(1);
			break;
		case 'p':	
			//decrease Kp by gain_interval 
			change_kp(-1);
			break;
		case 'D':		
			//increase Kd by gain_interval 
			change_kd(1);
			break;
		case 'd':		
			//decrease Kd by gain_interval 
			change_kd(-1);
			break;
		case 'I':		
			//increase Ki by gain_interval 
			change_ki(1);
			break;
		case 'i':		
			//decrease Ki by gain_interval 
			change_ki(-1);
			break;
		case 'T':		
		case 't':		
			// tell the pid_controller that trajectory should be executed
			//parse_trajectory(90, 360, 5);
	
			//do this in counts to avoid rounding
			parse_trajectory(562,-2249,31);

			break;
	}
	print_usb( MENU, MENU_LENGTH);

} //end menu()

//---------------------------------------------------------------------------------------
// Process received bytes. Loop through receiveBuffer and add new bytes (keystrokes)
// to another buffer for processing.
void check_for_new_bytes_received() {
	//flags
	int process_cmd;
	
	// check for bytes to add to the buffer
	int num_bytes_recvd = serial_get_received_bytes(USB_COMM);

	// BUG: when buffer gets to max buffer length, it "hiccups" and doesn't 
	// process the next character correctly
	// If nothing happens after hitting enter, hit enter a second time and
	// it typically runs
	// If the end of the command is cut off and not read in (i.e. r 1000 read in as r 100)
	// let the program run and re-enter the command at next prompt
	
	// I was having problems with occasional junk characters, but haven't seen
	// them in a while. Hopefully I fixed it while trying to fix the bug above.
	while(num_bytes_recvd != receive_buffer_pos) {
		// figure out how many new bytes are in receiveBuffer
		int num_bytes = num_bytes_recvd - tot_bytes_read;
		tot_bytes_read += num_bytes;
	
		// copy those bytes from receiveBuffer to menuBuffer
		strncat(menuBuffer, receiveBuffer+receive_buffer_pos,num_bytes);

		// calculate the new position of receive_buffer_pos
		// ring buffer will automatically start back at 0 if buffer is full
		// so receive_buffer_pos should do the same
		receive_buffer_pos += num_bytes;

		// known issue - when receive_buffer_pos starts back at 0
		// the newline isn't read correctly
		if (receive_buffer_pos >= sizeof(receiveBuffer)) {
			receive_buffer_pos = 0;
		}

		num_bytes_recvd = serial_get_received_bytes(USB_COMM);
		//lcd_goto_xy(0,0);
		//printf("BR: %d",num_bytes_recvd);
	}

	// keep copying until there is a newline value copied to the end of menuBuffer
	int len = strlen(menuBuffer);
	if (menuBuffer[len-1] == '\n' || menuBuffer[len-1] == '\r') {
		process_cmd = 1;
	} else {
		process_cmd = 0;
	}
	
	if (process_cmd) {
		//terminate string
		strcat(menuBuffer, "\0");
		
		//process buffer
		process_received_string(menuBuffer);

		//reset menuBuffer and flags
		memset(menuBuffer,0,sizeof(menuBuffer));
		process_cmd = 0;
	}
	
}
	
//-------------------------------------------------------------------------------------------
// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(USB_COMM))
		serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}


