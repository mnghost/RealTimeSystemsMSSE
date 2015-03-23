/* motor_lab - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 3/2/2015 9:20:09 PM
 *  Author: Seth Rowan
 * 
 * Program to run a motor plugged in into the Motor 1 port using a
 * PWM timer.
 */

#include <pololu/orangutan.h>
#include <stdbool.h>

int motor_speed = 0;
int motor_input = 0;
int direction = 1;

bool update_lcd = false;

static char g_last_m1a_val;
static char g_last_m1b_val;

static long g_counts_m1;

//Hold factor that converts percentage power to motor input (max 255)
const float power_conv = 255 / 100;

int main()
{
	//play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	//Set up timer 3 to interrupt at 5Hz. This will handle the encoder printout, as the main loop is way too fast
	//to update the LCD and get a clear display
	TCCR3A = (1 << COM3A1); //Clear compare match A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30); //CTC mode, with prescaler 64
	OCR3A = 62498; //Interrupt at 5Hz
	TIMSK3 = (1 << OCIE3A); //Enable interrupts from Timer 3
	
	//Set up timer 2 to drive the motor
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
	print("Spd: 0");

	//encoders_init(IO_D3, IO_D2, 0, 0);

	sei();
	
	while(1)
	{
		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		
		if(button == TOP_BUTTON)
		{
			if(motor_speed >= 95)
				motor_speed = 100;
			else
				motor_speed += 5;
				
			lcd_goto_xy(5,0);
			print("   ");
			lcd_goto_xy(5,0);
			print_long(motor_speed);			
		}
		
		if(button == BOTTOM_BUTTON)
		{
			if(motor_speed <= 5)
				motor_speed = 0;
			else
				motor_speed -= 5;
				
			lcd_goto_xy(5,0);
			print("   ");
			lcd_goto_xy(5,0);
			print_long(motor_speed);
		}
		
		if(button == MIDDLE_BUTTON)
		{			
			//direction = direction * -1;
			//set_m1_speed(0);
			//delay_ms(100);
			PORTC ^= (1 << PORTC7);
		}
	
		if(motor_speed == 0)
			TCCR2A &= ~(1 << COM2A1);
		else
			TCCR2A |= (1 << COM2A1);
	
		//motor_input = motor_speed * direction;
		//set_m1_speed(motor_input);
		motor_input = motor_speed * power_conv;
		if(motor_input > 255)
			motor_input = 255;
		
		OCR2A = motor_input;
		
		if (update_lcd)
		{
			lcd_goto_xy(0,1);
			print("                "); //Clear the line
			lcd_goto_xy(0,1);
			print_long(g_counts_m1);
			update_lcd = false;
		}
				
	}
}


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

ISR(TIMER3_COMPA_vect)
{
	//Update the LCD within the interrupt handler
	//This will allow the LCD to be more clear
	update_lcd = true;
}