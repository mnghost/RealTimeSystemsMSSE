/* Interrupt_lab - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 2/7/2015 11:08:08 AM
 *  Author: Seth
 */

#include <pololu/orangutan.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

unsigned int red_timer_counter = 0;
unsigned int red_user_spec = 1000;
volatile bool red_release = 0;
unsigned int yellow_timer_counter = 0;
unsigned int yellow_user_spec = 10;

volatile int _ii;
volatile int _jj;

#define RED_ON PORTA |= 0x04;
#define RED_OFF PORTA &= ~(0x04);
#define RED_TOGGLE PORTA ^= 0x04; G_red_toggles++;
#define YELLOW_ON PORTA |= 0x01;
#define YELLOW_OFF PORTA &= ~(0x01);
#define YELLOW_TOGGLE PORTA ^= (0x01); G_yellow_toggles++;
#define GREEN_ON PORTD |= 0x20;
#define GREEN_OFF PORTD &= ~(0x20);
#define GREEN_TOGGLE PORTD ^= (0x20); G_green_toggles++;

#define FOR_COUNT_10MS 10130
#define WAIT_10MS {for(_ii=0;_ii<FOR_COUNT_10MS;_ii++);}
	
#define MENU "\rMenu: {TPZ} {RGYA} <int>: "
#define MENU_LENGTH 27

// GLOBALS
volatile uint32_t G_red_toggles;
volatile uint32_t G_green_toggles;
volatile uint32_t G_yellow_toggles;

// local "global" data structures
char receive_buffer[32];
unsigned char receive_buffer_position;
char send_buffer[32];

// A generic function for whenever you want to print to your serial comm window.
// Provide a string and the length of that string. My serial comm likes "\r\n" at
// the end of each string (be sure to include in length) for proper linefeed.
void print_usb( char *buffer, int n ) {
	serial_send( USB_COMM, buffer, n );
	wait_for_sending_to_finish();
}
//------------------------------------------------------------------------------------------
// Initialize serial communication through USB and print menu options
// This immediately readies the board for serial comm
void init_menu() {
	char printBuffer[32];
	// Set the baud rate to 9600 bits per second. Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);
	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));
	//memcpy_P( send_buffer, PSTR("USB Serial Initialized\r\n"), 24 );
	//snprintf( printBuffer, 24, "USB Serial Initialized\r\n");
	//print_usb( printBuffer, 24 );
	print_usb( "USB Serial Initialized\r\n", 24);
	//memcpy_P( send_buffer, MENU, MENU_LENGTH );
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
	int value;
	int parsed;
	parsed = sscanf(buffer, "%c %c %d", &op_char, &color, &value);
	#ifdef ECHO2LCD
	lcd_goto_xy(0,0);
	printf("Got %c %c %d\n", op_char, color, value);
	#endif
	length = sprintf( tempBuffer, "Op:%c C:%c V:%d\r\n", op_char, color, value );
	print_usb( tempBuffer, length );
	// convert color to upper and check if valid
	color -= 32*(color>='a' && color<='z');
	switch (color) {
		case 'R':
		case 'G':
		case 'Y':
		case 'A': break;
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
			switch(color) {
				case 'R':
					red_user_spec = value;
					if(value == 0) {
						TIMSK0 = 0;
						RED_OFF;
					}
					else
						TIMSK0 = 0x02;
					break;
				case 'Y':
					yellow_user_spec = value / 100;
					if(value == 0)
						YELLOW_OFF;
					break;
				case 'G':
					ICR1 = (int)(19.53125 * value);
					OCR1A =  ICR1 / 2;
					break;
				case 'A':
					red_user_spec = value;
					yellow_user_spec = value / 100;
					ICR1 = (int)(19.53125 * value);
					OCR1A = ICR1 / 2;
					if(value == 0)
					{
						RED_OFF; YELLOW_OFF; GREEN_OFF;
					}
					break;
				default:
				break;
			}
		break;
		// print counter for <color> LED
		case 'P':
		case 'p':
		switch(color) {
			case 'R':
			length = sprintf( tempBuffer, "R toggles: %lu\r\n", G_red_toggles );
			print_usb( tempBuffer, length );
			break;
			case 'G':
			length = sprintf( tempBuffer, "G toggles: %lu\r\n", G_green_toggles );
			print_usb( tempBuffer, length );
			break;
			case 'Y':
			length = sprintf( tempBuffer, "Y toggles: %lu\r\n", G_yellow_toggles );
			print_usb( tempBuffer, length );
			break;
			case 'A':
			length = sprintf( tempBuffer, "Toggles R:%lu G:%lu Y:%lu\r\n", G_red_toggles, G_green_toggles, G_yellow_toggles );
			print_usb( tempBuffer, length );
			break;
			default: print_usb("Default in p(color). How?\r\n", 27 );
		}
		break;
		// zero counter for <color> LED
		case 'Z':
		case 'z':
		switch(color) {
			case 'R': G_red_toggles=0; TCNT0=0; break;
			case 'G': G_green_toggles=0; TCNT1=0; break;
			case 'Y': G_yellow_toggles=0; TCNT3=0; break;
			case 'A': G_red_toggles = G_green_toggles = G_yellow_toggles = 0; TCNT0=TCNT1=TCNT3=0; break;
			default: print_usb("Default in z(color). How?\r\n", 27 );
		}
		break;
		default:
		print_usb( "Command does not compute.\r\n", 27 );
	} // end switch(op_char)
	print_usb( MENU, MENU_LENGTH);
} //end menu()
//---------------------------------------------------------------------------------------
// If there are received bytes to process, this function loops through the receive_buffer
// accumulating new bytes (keystrokes) in another buffer for processing.
void check_for_new_bytes_received()
{
	/*
	The receive_buffer is a ring buffer. The call to serial_check() (you should call prior to this function) fills the buffer.
	serial_get_received_bytes is an array index that marks where in the buffer the most current received character resides.
	receive_buffer_position is an array index that marks where in the buffer the most current PROCESSED character resides.
	Both of these are incremented % (size-of-buffer) to move through the buffer, and once the end is reached, to start back at the beginning.
	This process and data structures are from the Pololu library. See examples/serial2/test.c and src/OrangutanSerial/*.*
	A carriage return from your comm window initiates the transfer of your keystrokes.
	All key strokes prior to the carriage return will be processed with a single call to this function (with multiple passes through this loop).
	On the next function call, the carriage return is processes with a single pass through the loop.
	The menuBuffer is used to hold all keystrokes prior to the carriage return. The "received" variable, which indexes menuBuffer, is reset to 0
	after each carriage return.
	*/
	char menuBuffer[32];
	static int received = 0;
	// while there are unprocessed keystrokes in the receive_buffer, grab them and buffer
	// them into the menuBuffer
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
	{
	// place in a buffer for processing
	menuBuffer[received] = receive_buffer[receive_buffer_position];
	++received;
	// Increment receive_buffer_position, but wrap around when it gets to
	// the end of the buffer.
	if ( receive_buffer_position == sizeof(receive_buffer) - 1 )
	{
	receive_buffer_position = 0;
	}
	else
	{
	receive_buffer_position++;
	}
	}
	// If there were keystrokes processed, check if a menue command
	if (received) {
	// if only 1 received, it was MOST LIKELY a carriage return.
	// Even if it was a single keystroke, it is not a menu command, so ignore it.
	if ( 1 == received ) {
	received = 0;
	return;
	}
	// Process buffer: terminate string, process, reset index to beginning of array to receive another command
	menuBuffer[received] = '\0';
	#ifdef ECHO2LCD
	lcd_goto_xy(0,1);
	print("RX: (");
	print_long(received);
	print_character(')');
	for (int i=0; i<received; i++)
	{
	print_character(menuBuffer[i]);
	}
	#endif
	process_received_string(menuBuffer);
	received = 0;
	}
	}
	//-------------------------------------------------------------------------------------------
	// wait_for_sending_to_finish: Waits for the bytes in the send buffer to
	// finish transmitting on USB_COMM. We must call this before modifying
	// send_buffer or trying to send more bytes, because otherwise we could
	// corrupt an existing transmission.
	void wait_for_sending_to_finish()
	{
	while(!serial_send_buffer_empty(USB_COMM))
	serial_check(); // USB_COMM port is always in SERIAL_CHECK mode
	}



int main()
{
	//play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.
	clear();
	
	/******************************************
	* Configure Timer 0, an 8-bit timer
	* Runs red LED
	*******************************************/
	//Initialize Timer/Counter 0 Control Register A
	//Clear OC0A on compare match, use CTC mode
	TCCR0A = 0x82; //10000010, refer to page 101 of data sheet for config
	//Initialize Timer/Control 0 Control Register B 
	TCCR0B = 0x04;//100, set prescaler to 256
	TIMSK0 = 0x02; //010, Enable interrupts from Timer 0
	
	//This will cause the interrupt to occur at approx. 1KHz with prescaler 256
	OCR0A = 77; //Match OC0A with value of OCR0A
	
	/******************************************
	* Configure Timer 1, a 16-bit timer
	* The Compare Register A for this timer is
	* tied to PD5, per data sheet pg. 3
	* Runs green LED
	*******************************************/
	TCCR1A = 0xC2; //11000010, Set on up count, clear on down count
	//TCCR1B = 0x1B; //000111011, Fast PWM mode, prescaler = 64, Top = ICR1
	//TCCR1B = 0X1C; //000111100, Fast PWM mode prescaler = 256, Top = ICR1
	TCCR1B = 0x15; //00010101, Phase correct PWM mode, prescaler = 1024, Top = ICR1
	
	//Set a 50% duty cycle
	ICR1 = 5000; //Top of the wave form
	OCR1A = ICR1 / 2; //Compare Register A value
	TIMSK1 = 0x02; //Enable interrupt for both Comp A and Overflow
	
	/*****************************************
	* Configure Timer 3
	* Use CTC mode with a prescaler of 1024
	* Runs yellow LED
	******************************************/
	TCCR3A = 0x80; //Clear compare match A
	TCCR3B = 0x0B; //CTC mode, with prescaler 64
	OCR3A = 31249; //Interrupt at 10Hz
	TIMSK3 = 0x02; //Enable interrupts from Timer 3
	
	DDRD = 0x20; //Enable pin 5 on port D, 1 << PD5
	DDRA = 0x05; //Enables pins 0 and 2 on port A
	
	/*********************************/
	//* This code was used to capture the exact number of FOR LOOP cycles
	//* to get exactly 10 ms
		//lcd_init_printf();
		//clear();
	//
		//unsigned long time_before = get_ms();
		//WAIT_10MS
		//unsigned long time_after = get_ms();
			//
		//printf("%lu-",time_after-time_before);
	/***********************************/
	
	lcd_init_printf();
	init_menu();
	
	sei(); //Enable global interrupts

	//Set to 500ms blink period
	red_user_spec = 500;
	yellow_user_spec = 5;
	ICR1 = 9765;
	OCR1A = ICR1 / 2;
	
	TCNT0 = 0; //Red
	TCNT1 = 0; //Green
	TCNT3 = 0; //Yellow
	
	while(1)
	{
		if (red_release == 1)
		{
			RED_TOGGLE;
			red_release = 0;
		}
		
		//for ( _jj=0; _jj < red_user_spec/10; _jj++)
		//{
			//WAIT_10MS;
		//}
		//RED_TOGGLE;
		
		serial_check();
		check_for_new_bytes_received();
	}
}

//Interrupt handler for Timer 0, compare register A
//Fires at 1000 Hz
ISR(TIMER0_COMPA_vect)
{
	
	if(red_timer_counter % red_user_spec == 0)
	{
		red_release = 1;	
		red_timer_counter = 0;
	}

	red_timer_counter++;
}

//Fires at user-specified value
ISR(TIMER1_COMPA_vect)
{

	if (ICR1 != 0)
		G_green_toggles++;
}

//Fires at 10 Hz
ISR(TIMER3_COMPA_vect)
{
	sei();

	//Wait 510 ms
	for(_jj=0;_jj<51;_jj++)
	{
		WAIT_10MS;
	}

	if (yellow_timer_counter % yellow_user_spec == 0)
	{
		YELLOW_TOGGLE;
		yellow_timer_counter = 0;
	}
	
	yellow_timer_counter++;
	
}


