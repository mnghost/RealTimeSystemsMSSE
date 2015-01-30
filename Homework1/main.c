/* Homework1 - an application for the Pololu Orangutan SVP
 *
 * This application uses the Pololu AVR C/C++ Library.  For help, see:
 * -User's guide: http://www.pololu.com/docs/0J20
 * -Command reference: http://www.pololu.com/docs/0J18
 *
 * Created: 1/26/2015 9:35:51 PM
 *  Author: Seth Rowan
 */

#include <pololu/orangutan.h>
#include <stdbool.h>

//Function to make the red LED blink
void flash_red(unsigned int speed)
{
	unsigned long time_now;
	static unsigned long last_time;
	static unsigned long elapsed_time = 0;
	static unsigned int light_on = 0;
	
	time_now = get_ms();
	
	elapsed_time += time_now - last_time;
	
	last_time = time_now;
	
	//If 200 ms or more have passed, toggle the light
	if(elapsed_time >= (500 / speed))
	{
		if(light_on)
			light_on = 0;
		else
			light_on = 1;
		elapsed_time = 0;
	}
	
	red_led(light_on);
}

//Function to make the green LED blink
void flash_green(unsigned int speed)
{
	unsigned long time_now;
	static unsigned long last_time;
	static unsigned long elapsed_time = 0;
	static unsigned int light_on = 0;
	
	time_now = get_ms();
	
	elapsed_time += time_now - last_time;
	
	last_time = time_now;
	
	//If 500 ms or more have passed, toggle the light
	if(elapsed_time >= (1000 / speed))
	{
		if(light_on)
			light_on = 0;
		else
			light_on = 1;
		elapsed_time = 0;
	}
	
	green_led(light_on);	
}


int main()
{
	play_from_program_space(PSTR(">g32>>c32"));  // Play welcoming notes.

	bool red_on = false;
	bool green_on = false;
	unsigned int blink_rate = 1;

	clear();
	print("Homework 1");
	lcd_goto_xy(0,1);
	print("Seth Rowan");
	
	//The green LED comes on dim after the LCD has been updated.
	//Let's clear that right now.	
	green_led(0);
	delay_ms(1000);

    clear();
	print("Seth Rowan");
	lcd_goto_xy(0,1);
	print("Speed: 1");

	while(1)
	{
		//When the top button is pressed, toggle whether the red LED
		//should be flashing or not
		unsigned char button = get_single_debounced_button_press(ANY_BUTTON);
		
		if(button == TOP_BUTTON)
		{
			if(red_on)
			{
				red_on = false;
				red_led(0);
			}
			else
				red_on = true;
		}

		//When bottom button is pressed, toggle whether the green LED
		//should be flashing or not
		if(button == BOTTOM_BUTTON)
		{
			if(green_on)
			{
				green_on = false;
				green_led(0);
			}
			else
				green_on = true;
		}
		
		//Use the middle button to cycle through the blink rate setting
		if(button == MIDDLE_BUTTON)
		{
			switch(blink_rate)
			{
				case 1:
					blink_rate = 2;
					break;
				case 2:
					blink_rate = 3;
					break;
				case 3:
					blink_rate = 4;
					break;
				case 4:
					blink_rate = 5;
					break;
				case 5:
					blink_rate = 1;

			}
			lcd_goto_xy(7,1);
			print_long(blink_rate);
		}
		
		//Start blinking the red LED if it is on
		if(red_on)
		{
			flash_red(blink_rate);
		}
		
		//Start blinking the green LED if it is on
		if(green_on)
		{
			flash_green(blink_rate);
		}
	}
}
