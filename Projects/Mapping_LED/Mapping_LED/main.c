/* Mapping_LED - an application for the Pololu Orangutan SVP
 *
 * Assignment 2 
 * Program makes an external LED plugged into I/O port D3 or the on-board red LED start/stop blinking
 * by pressing the bottom or middle on-board buttons, respectively. The program can also take input
 * from a PC terminal such that pressing the - key decreases the blink rate, and pressing the + key
 * increases the blink rate. The top on-board button selects which LED's blink rate gets modified.
 *
 * LEDs are addressed using their DDR addresses instead of the Pololu libraries.
 *
 * Portions controlling input from the PC were stolen shamelessly from Pololu example program "serial2"
 *
 * Created: 1/30/2015 11:07:29 AM
 *  Author: Seth Rowan
 */

#include <pololu/orangutan.h>
#include <stdbool.h>

// receive_buffer: A ring buffer that we will use to receive bytes on USB_COMM.
// The OrangutanSerial library will put received bytes in to
// the buffer starting at the beginning (receiveBuffer[0]).
// After the buffer has been filled, the library will automatically
// start over at the beginning.
char receive_buffer[32];

// receive_buffer_position: This variable will keep track of which bytes in the receive buffer
// we have already processed.  It is the offset (0-31) of the next byte
// in the buffer to process.
unsigned char receive_buffer_position = 0;

// send_buffer: A buffer for sending bytes on USB_COMM.
char send_buffer[32];

unsigned int selected_led = 1; //Tracks which LED is selected when modifying the blink rate
unsigned int ext_speed = 300; //Blink rate for external LED
unsigned int obl_speed = 300; //Blink rate for on-board LED

// wait_for_sending_to_finish:  Waits for the bytes in the send buffer to
// finish transmitting on USB_COMM.  We must call this before modifying
// send_buffer or trying to send more bytes, because otherwise we could
// corrupt an existing transmission.
void wait_for_sending_to_finish()
{
	while(!serial_send_buffer_empty(USB_COMM))
	serial_check();		// USB_COMM port is always in SERIAL_CHECK mode
}


void decrease_rate()
{
	switch (selected_led)
	{
		case 1:
			if(ext_speed < 32740)
				ext_speed += 20;
			else
				ext_speed = 32760;
			break;
		case 2:
			if(ext_speed < 32740)
				obl_speed += 20;
			else
				obl_speed = 32760;
			break;
	}
}

void increase_rate()
{
	switch (selected_led)
	{
		case 1:
			if(ext_speed >= 20)
				ext_speed -= 20;
			else
				ext_speed = 0;
			break;
		case 2:
			if(obl_speed >= 20)
				obl_speed -= 20;
			else
				obl_speed = 0;
			break;
	}
}

// process_received_byte: Responds to a byte that has been received on
// USB_COMM.  If you are writing your own serial program, you can
// replace all the code in this function with your own custom behaviors.
void process_received_byte(char byte)
{
	switch(byte)
	{
		// If the character '-' or '_' is received, reduce the blink rate.
		case '-':
		case '_':
			lcd_goto_xy(0,1);
			print("Slower");
			decrease_rate();
			break;
		
		// If the character '=' or '+' is received, increase the bloink rate.
		case '+':
		case '=':
			lcd_goto_xy(0,1);
			print("Faster");
			increase_rate();
			break;

	}
}

void check_for_new_bytes_received()
{
	while(serial_get_received_bytes(USB_COMM) != receive_buffer_position)
	{
		// Process the new byte that has just been received.
		process_received_byte(receive_buffer[receive_buffer_position]);

		// Increment receive_buffer_position, but wrap around when it gets to
		// the end of the buffer.
		if (receive_buffer_position == sizeof(receive_buffer)-1)
		{
			receive_buffer_position = 0;
		}
		else
		{
			receive_buffer_position++;
		}
	}
}

//Blinks the external LED plugged into Port D, Pin 3
void blink_ext_led()
{
	unsigned long time_now;
	static unsigned long last_time;
	static unsigned long elapsed_time = 0;
	static bool led_on = false;

	time_now = get_ms();

	elapsed_time += time_now - last_time;

	last_time = time_now;

	//If 200 ms or more have passed, toggle the light
	if(elapsed_time >= ext_speed)
	{
		if(led_on)
		{
			led_on = false;
			PORTD |= 0x08;  //0x08 equates to 00001000
		}
		else
		{
			led_on = true;
			PORTD &= ~(0x08);
		}
		elapsed_time = 0;
	}
}

//Blinks the on-board red connected to Port D, Pin 1
void blink_onboard_led()
{
	unsigned long time_now;
	static unsigned long last_time;
	static unsigned long elapsed_time = 0;
	static bool led_on = false;

	time_now = get_ms();

	elapsed_time += time_now - last_time;

	last_time = time_now;

	//If 200 ms or more have passed, toggle the light
	if(elapsed_time >= obl_speed)
	{
		if(led_on)
		{
			led_on = false;
			PORTD |= 0x02;  //0x08 equates to 00000010
		}
		else
		{
			led_on = true;
			PORTD &= ~(0x02);
		}
		elapsed_time = 0;
	}	
}

//Make sure the external LED in D3 is off
void force_ext_off()
{
	PORTD &= ~(0x08);
}

//Make sure the on-board LED on D1 is off.
//This LED works opposite of typical peripherals, it is 
//off when it receives signal and on when it does not
void force_obl_off()
{
	PORTD |= (0x02);
}


int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	// Set the baud rate to 9600 bits per second.  Each byte takes ten bit
	// times, so you can get at most 960 bytes per second at this speed.
	serial_set_baud_rate(USB_COMM, 9600);

	// Start receiving bytes in the ring buffer.
	serial_receive_ring(USB_COMM, receive_buffer, sizeof(receive_buffer));

    //Set all of Port D to output
	DDRD = 0xFF;

	bool ext_is_blinking = false; //Track state of external LED
	bool obl_is_blinking = false; //Track state of on-board LED
	
	clear();
	lcd_goto_xy(0,0);
	print("LED: External");
	
	while(1)
	{
		// USB_COMM is always in SERIAL_CHECK mode, so we need to call this
		// function often to make sure serial receptions and transmissions
		// occur.
		serial_check();

		// Deal with any new bytes received.
		check_for_new_bytes_received();
		
		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);

		//Toggle the blinking of the external LED
		if(button == BOTTOM_BUTTON)
		{
			if(ext_is_blinking)
				ext_is_blinking = false;
			else
				ext_is_blinking = true;
		}
		
		//Toggle the blinking of the on-board LED
		if(button == MIDDLE_BUTTON)
		{
			if(obl_is_blinking)
				obl_is_blinking = false;
			else
				obl_is_blinking = true;
		}
		
		if(button == TOP_BUTTON)
		{
			switch(selected_led)
			{
				case 1:
					selected_led = 2;
					lcd_goto_xy(0,0);
					print("LED: On-board");
					break;
				case 2:
					selected_led = 1;
					lcd_goto_xy(0,0);
					print("LED: External");
					break;
			}
		}
		
		
		if(ext_is_blinking)
			blink_ext_led();
		else
			force_ext_off();
			
		if(obl_is_blinking)
			blink_onboard_led();
		else
			force_obl_off();
			
		//DDRD = 1 << PORTD3; 

	}
}
