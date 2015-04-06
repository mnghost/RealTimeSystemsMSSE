/* pid_control - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 3/22/2015 9:48:52 PM
 *  Author: Seth
 *
 * PID controller for both position and speed
 */

#define ECHO2LCD

#include <pololu/orangutan.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>

#include "menu.h"

//Initialize the globals
int g_motor_signal = 0;
long g_reference_val = 0;
long g_current_val = 0;
int g_Kp = 1;
int g_Ki = 1;
int g_Kd = 1;
char g_pid_mode = 'S';
bool g_logging = false;
bool g_execute = false;
bool g_log_output = false;
bool g_program_mode = false;

long g_last_position = 0;
long g_curr_position = 0;
int g_last_velocity = 0;
int g_curr_velocity = 0;
long g_error = 0;
long g_errorSum = 0;
long g_errorDelta = 0;

bool update_lcd = false;

static char g_last_m1a_val;
static char g_last_m1b_val;

static long g_counts_m1;

int main()
{
	//play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	/**********************************************************************************************
	* Set up timer 3 to interrupt at 5Hz. This will handle the encoder printout, as the main loop is way too fast
	* to update the LCD and get a clear display
	**********************************************************************************************/
	TCCR3A = (1 << COM3A1); //Clear compare match A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30); //CTC mode, with prescaler 64
	OCR3A = 31249; //Interrupt at 10Hz
	TIMSK3 = (1 << OCIE3A); //Enable interrupts from Timer 3
	
	/**********************************************************************************************
	* Set up timer 1 interrupt to handle the PID calculation.
	**********************************************************************************************/
	TCCR1A = (1 << COM1A1); //Clear compare match A
	TCCR1B = (1 << WGM12) | (1 << CS11); //CTC mode, with prescaler 8
	//TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); //CTC mode, prescaler 64
	OCR1A = 25000; //Interrupt at 100Hz with prescaler 8
	//OCR1A = 12500; //Interrupt at 200Hz with prescaler 8
	//OCR1A = 12500; //Interrupt at 25 Hz with prescaler 64
	TIMSK1 = (1 << OCIE1A); //Enable interrupts from Timer 1
	
	/************************************************************
	*Set up timer 2 to drive the motor with PWM
	************************************************************/
	TCCR2A = (1 << WGM21) || (1 << WGM20); //Fast PWM mode
	TCCR2B = (1 << CS21); //Prescaler 8
	OCR2A = 0; //Initialize to lowest setting
	
	//Set motor and direction pins to output
	DDRD |= (1 << PORTD7) | (1 << PORTD6);
	DDRC |= (1 << PORTC7) | (1 << PORTC6);
	
	//Enable pin-change interrupt for pins D2 and D3,
	//Which corresponds to PCINT26 AND PCINT27
	PCICR = (1 << PCIE3); //Enable interrupt for bank 24-31
	PCMSK3 = (1 << PCINT26) | (1 << PCINT27); //Check pins D2 and D3

	clear();
	lcd_goto_xy(0,0);
	print("Ref: 0");
	lcd_goto_xy(0,1);
	print("Mez: 0");

	init_menu();

	sei();
	
	int motor_input;
	
	//Variables for the program mode
	short position = 1;
	long start_ms;
	long curr_ms;
	bool on_target_prev = false;
	
	while(1)
	{
		serial_check();
		check_for_new_bytes_received();
		
		//Run in program mode, rather than free form S or R modes
		if(g_program_mode)
		{
			switch (position)
			{
				case 1:
					//Rotate 90 degrees from count 0
					g_reference_val = 564;
					break;
				case 2:
					//Rotate 360 degrees back from count 564
					g_reference_val = -1692;
					break;
				case 3:
					//Rotate 5 degrees forward from count -1692;
					g_reference_val = -1660;
					break;
				default:
					//End the execution and reset
					g_execute = false;
					position = 1;
			}
			
			//Are we at the reference position? (Or at least within 5 ticks of it?)
			if (g_current_val <= (g_reference_val + 5) && g_current_val >= (g_reference_val - 5)) //Needs to be used with low gains
			//if (g_current_val == g_reference_val)
			{
				//If this is our first time at the target since last cycle;
				if (!on_target_prev)
				{
					start_ms = get_ms(); //Start the timer for how long we are on position	
					on_target_prev = true;
				}
				else
				{
					curr_ms = get_ms();
					
					//Check if we have been at this position for half a second
					if (curr_ms - start_ms >= 1000)
					{
						position++; //Move onto the next position
						on_target_prev = false;
					}
				}
			}
			else //We are not at the reference, either not yet, or we passed it
			{
				on_target_prev = false; //Reset this
			}
		}
		
		
		if(g_motor_signal == 0)
			TCCR2A &= ~(1 << COM2A1);
		else
			TCCR2A |= (1 << COM2A1);
		
		if(g_motor_signal < 0)
		{
			motor_input = g_motor_signal * -1;
			PORTC |= (1 << PORTC7);
		}
		else
		{
			motor_input = g_motor_signal;
			PORTC &= ~(1 << PORTC7);
		}
		
		if(motor_input > 255)
			motor_input = 255;
		
		OCR2A = motor_input;
		
		if (update_lcd)
		{
			lcd_goto_xy(5,0);
			print("           "); //Clear the line
			lcd_goto_xy(5,0);
			print_long(g_reference_val);
			lcd_goto_xy(5,1);
			print("           "); //Clear the line
			lcd_goto_xy(5,1);
			if(g_pid_mode == 'S'){
				print_long(g_curr_velocity);
			}
			if(g_pid_mode == 'R'){
				print_long(g_curr_position);
			}
			update_lcd = false;
			lcd_goto_xy(15,1);
			print_character(g_pid_mode);
			lcd_goto_xy(15,0);
			print_long(position);
		}
		
		if (g_log_output)
		{
			char tempBuffer[45];
			int length = sprintf(tempBuffer, "%li | %li | %i | %li | %li\n\r", g_reference_val, g_current_val, g_motor_signal, g_error, g_errorSum);
			print_usb(tempBuffer, length);
			
			g_log_output = false;
		}
	}
}

//Interrupt to count the pins changes for the encoder
ISR(PCINT3_vect)
{
	unsigned char m1_chanA;
	unsigned char m1_chanB;
	
	if (PIND & (1 << PD3))
	m1_chanA = 1;
	else
	m1_chanA = 0;
	
	if (PIND & (1 << PD2))
	m1_chanB = 1;
	else
	m1_chanB = 0;
	
	char plus_m1 = m1_chanA ^ g_last_m1b_val;
	char minus_m1 = m1_chanB ^ g_last_m1a_val;
	
	if(plus_m1)
	g_counts_m1 += 1;
	if(minus_m1)
	g_counts_m1 -= 1;
	
	g_last_m1a_val = m1_chanA;
	g_last_m1b_val = m1_chanB;
}

//Fires 10 times a second, handles LCD update
ISR(TIMER3_COMPA_vect) 
{
	update_lcd = true;
}

//Do PID calculation
ISR(TIMER1_COMPA_vect)
{	
	//Execute the trajectory
	if (g_execute == true)
	{
		//Divide the inputs by 1000 here because i could not figure out
		//how to do floats with my I/O
		float fl_Kp = (float)g_Kp / 1000.0;
		float fl_Ki = (float)g_Ki / 1000.0;
		float fl_Kd = (float)g_Kd / 1000.0;
	
		//Where are we now?
		g_curr_position = g_counts_m1;
		g_curr_velocity = g_curr_position - g_last_position;
	
		//Looking for a target speed
		if (g_pid_mode == 'S')
		{
			g_error = g_reference_val - g_curr_velocity;
			g_errorDelta = g_last_velocity - g_curr_velocity;
		
			g_current_val = g_curr_velocity;
		}
	
		//Looking for a target position
		if (g_pid_mode == 'R')
		{
			g_error = g_reference_val - g_curr_position;
			g_errorDelta = g_last_position - g_curr_position;
		
			g_current_val = g_curr_position;
		}
	
		// Here's I
		g_errorSum += g_error;
	
		//Don't let I run away, or it does not stabilize
		if(g_errorSum > 1000)
			g_errorSum = 1000;
		
		if(g_errorSum < -1000)
			g_errorSum = -1000;
	
		//Do the PID calculation
		g_motor_signal = (int)((float)(fl_Kp * g_error) + (float)(fl_Ki * g_errorSum) + (float)(fl_Kd * g_errorDelta));
	
		//Motor has a maximum power of 255
		//Don't let the motor signal get too big
		if(g_motor_signal > 255)
			g_motor_signal = 255;
	
		//Or too small
		if(g_motor_signal < -255)
			g_motor_signal = -255;

	
		g_last_position = g_curr_position;
		g_last_velocity = g_curr_velocity;
		
		//Set the flag to push output to the terminal window
		if (g_logging)
		{
			g_log_output = true;
		}
	}
	else //Just reset everything
	{
		g_motor_signal = 0;
		g_error = 0;
		g_errorDelta = 0;
		g_errorSum = 0;
		g_curr_position = 0;
		g_curr_velocity = 0;
		g_last_position = 0;
		g_last_velocity = 0;
		g_counts_m1 = 0;
	}
}