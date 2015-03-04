#include "menu.h"
#include "led.h"
#include "timer.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <pololu/orangutan.h>  

// GLOBALS
extern uint32_t R_TOGGLE;
extern uint32_t G_TOGGLE;
extern uint32_t Y_TOGGLE;


// local "global" data structures
char receive_buffer[32];
unsigned char receive_buffer_position = 0;
char send_buffer[32];

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
	
	//char printBuffer[32];
	
	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

	print_usb( "USB Serial Initialized\r\n", 24);

	print_usb( MENU, MENU_LENGTH );
}

//------------------------------------------------------------------------------------------
// process_received_byte: Parses a menu command (series of keystrokes) that 
// has been received on USB_COMM and processes it accordingly.
// The menu command is buffered in check_for_new_bytes_received (which calls this function).
void process_received_string(const char* buffer)
{
	// Used to pass to USB_COMM for serial communication
	int length;
	char tempBuffer[32];
	
	// parse and echo back to serial comm window (and optionally the LCD)
	char color;
	char op_char;
	int value = 0;
	int parsed;
	parsed = sscanf(buffer, "%c %c %d", &op_char, &color, &value);

#ifdef ECHO2LCD
	lcd_goto_xy(0,0);
	printf("Got %c %c %d", op_char, color, value);
#endif
	length = sprintf( tempBuffer, "Op:%c C:%c V:%d\r\n", op_char, color, value );
	print_usb( tempBuffer, length );

	
	// convert color to upper and check if valid
	int thisColor;
	color -= 32*(color>='a' && color<='z');
	switch (color) {
		case 'R':
			thisColor = RED;
			break;
		case 'G':
			thisColor = GREEN;
			break;
		case 'Y': 
			thisColor = YELLOW;
			break;
		case 'A': 
			thisColor = ALL;
			break;
		default:
			print_usb( "Bad Color. Try {RGYA}\r\n", 23 );
			print_usb( MENU, MENU_LENGTH);
			return;
	}

	// Check valid command and implement
	switch (op_char) {
		// change toggle frequency for <color> LED
		case 'T':
		case 't':
			set_toggle(thisColor,value);
			break; 
		// print counter for <color> LED 
		case 'P':
		case 'p':
			switch(color) {
				case 'R': 
					length = sprintf( tempBuffer, "R toggles: %d\r\n", (int)R_TOGGLE );
					print_usb( tempBuffer, length ); 
					break;
				case 'G': 
					length = sprintf( tempBuffer, "G toggles: %d\r\n", (int)G_TOGGLE );
					print_usb( tempBuffer, length ); 
					break;
				case 'Y': 
					length = sprintf( tempBuffer, "Y toggles: %d\r\n", (int)Y_TOGGLE );
					print_usb( tempBuffer, length ); 
					break;
				case 'A': 
					length = sprintf( tempBuffer, "Toggles R:%d G:%d Y:%d\r\n", (int)R_TOGGLE, (int)G_TOGGLE, (int)Y_TOGGLE );
					print_usb( tempBuffer, length ); 
					break;
				default: print_usb("Default in p(color). How?\r\n", 27 );
			}
			break;

		// zero counter for <color> LED 
		case 'Z':
		case 'z':
			switch(color) {
				case 'R': R_TOGGLE=0; break;
				case 'G': G_TOGGLE=0; break;
				case 'Y': Y_TOGGLE=0; break;
				case 'A': R_TOGGLE = G_TOGGLE = Y_TOGGLE = 0; break;
				default: print_usb("Default in z(color). How?\r\n", 27 );
			}
			break;
		default:
			print_usb( "Command does not compute.\r\n", 27 );
		} // end switch(op_char) 
		
	print_usb( MENU, MENU_LENGTH);

} //end menu()

//---------------------------------------------------------------------------------------
// Process received bytes. Loop through receive_buffer and add new bytes (keystrokes)
// to another buffer for processing.
void check_for_new_bytes_received() {
	char tempBuffer[32] = "";
	int tempLength = 0;

	//this buffer contains what needs to be processed
	char menuBuffer[32];

	//flags
	static int received = 0;
	
	// check for bytes to add to the buffer
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position) {
		
		// place in a buffer for processing
		strncat(menuBuffer, receive_buffer+receive_buffer_position,1);
		received++;
		
		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer. 
		if ( receive_buffer_position == sizeof(receive_buffer) - 1 ){
			receive_buffer_position = 0;
		} else {
			receive_buffer_position++;
		}

	}

	// If there were keystrokes processed, check if a menue command
	if (received) {
		if (1==received) {
			received = 0;
			return;
		}

		int len = strlen(menuBuffer);
		if (menuBuffer[len-1]=='\n') {
			menuBuffer[len-1] = '\0';
		}
		// Process buffer: terminate string, process, reset index to beginning of array to receive another command
		strcat(menuBuffer,"\0");

		lcd_goto_xy(0,1);
		printf("RX (%d): %s",received, menuBuffer);
		
		tempLength = sprintf( tempBuffer, "\nRX (%d): %s\n",received, menuBuffer);
		print_usb(tempBuffer,tempLength);

		process_received_string(menuBuffer);
		memset(menuBuffer,0,sizeof(menuBuffer));
		received = 0;
		//memset(receive_buffer,0,sizeof(menuBuffer));
		//receive_buffer_position = 0;
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


