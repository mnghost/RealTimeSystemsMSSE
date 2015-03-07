/* motor_lab - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 3/2/2015 9:20:09 PM
 *  Author: Seth
 */

#include <pololu/orangutan.h>

int motor_speed = 0;
int motor_input = 0;
int direction = 1;

int main()
{
	//play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	//Set up timer to interrupt at 5Hz. This will handle the encoder printout, as the main loop is way too fast
	//to update the LCD and get a clear display
	TCCR3A = 0x80; //Clear compare match A
	TCCR3B = 0x0B; //CTC mode, with prescaler 64
	OCR3A = 62498; //Interrupt at 5Hz
	TIMSK3 = 0x02; //Enable interrupts from Timer 3

	clear();
	lcd_goto_xy(0,0);
	print("Spd: 0");
	green_led(0);

	encoders_init(IO_D3, IO_D2, 0, 0);

	while(1)
	{
		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		
		if(button == TOP_BUTTON)
		{
			if(motor_speed >= 250)
				motor_speed = 255;
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
			direction = direction * -1;
		}
	
		motor_input = motor_speed * direction;
		set_m1_speed(motor_input);
				
	}
}

ISR(TIMER3_COMPA_vect)
{
	//Update the LCD within the interrupt handler
	//This will allow the LCD to be more clear
	lcd_goto_xy(0,1);
	print("                "); //Clear the line
	lcd_goto_xy(0,1);
	print_long(encoders_get_counts_m1());	
}