/*
 * Project.c
 *
 *  Created on: Jan 28, 2024
 *      Author: Marwan Medhat
 */

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>

#define SET_BIT(REG,BIT) REG |= (1<<BIT)
#define CLEAR_BIT(REG,BIT) REG &= ~(1<<BIT)
#define ONE_SECOND 15625 /* 15625 tick is equivalent to 1 second using 64 prescaler */
#define SEVEN_SEGMENT_UPPER_LIMIT 10
#define SECONDS_UPPER_LIMIT 6
#define MINUTES_UPPER_LIMIT 6
#define HOURS0_UPPER_LIMIT 4
#define HOURS1_UPPER_LIMIT 2

/* global variables to use in ISRS and main function */
unsigned char sec0 = 0, sec1 = 0, min0 = 0, min1 = 0, hr0 = 0, hr1 = 0;

void INT0_On(void)
{
	CLEAR_BIT(DDRD,2); /* set pin 2 in PORTB as input */
	SET_BIT(MCUCR,ISC01);
	SET_BIT(GIFR,INTF0);
	SET_BIT(GICR,INT0);
	SET_BIT(PORTD,2); /* This will activate pull-up resistor */
}

void INT1_On(void)
{
	CLEAR_BIT(DDRD,3); /* set pin 3 in PORTD as input */
	SET_BIT(MCUCR,ISC10);
	SET_BIT(MCUCR,ISC11);
	SET_BIT(GIFR,INTF1);
	SET_BIT(GICR,INT1);
}

void INT2_On(void)
{
	CLEAR_BIT(DDRB,2); /* set pin 2 in PORTD as input */
	CLEAR_BIT(MCUCSR,ISC2);
	SET_BIT(GIFR,INTF2);
	SET_BIT(GICR,INT2);
	SET_BIT(PORTB,2); /* This will activate pull-up resistor */
}

void TIMER1_Comp_On(void)
{
	/*
	 * To adjust TIMER1 on compare mode
	 * 1- FOC1A and FOC1B is set for compare mode
	 * 2- WGM12 bit is set for compare mode with OCR1A register as compare value
	 * 3- CS11 and CS10 bits are set for 64 prescaler
	 */

	TCCR1A |= (1<<FOC1A) | (1<<FOC1B);
	TCCR1B |= (1<<WGM12) | (1<<CS11) | (1<<CS10);
	TCNT1 = 0; /* initial tick is 0 */
	OCR1A = ONE_SECOND;
	SET_BIT(TIMSK,OCIE1A); /* Compare Match A interrupt is enabled */
}

ISR(TIMER1_COMPA_vect)
{
	sec0++; /* interrupt every second */
}

ISR(INT0_vect)
{
	sec0 = 0;
	sec1 = 0;
	min0 = 0;
	min1 = 0;
	hr0 = 0;
	hr1 = 0;
}

ISR(INT1_vect)
{
	CLEAR_BIT(TCCR1B,CS11);
	CLEAR_BIT(TCCR1B,CS10);
}

ISR(INT2_vect)
{
	SET_BIT(TCCR1B,CS11);
	SET_BIT(TCCR1B,CS10);
}


int main()
{

	SET_BIT(SREG,7); /* I-Bit is now on */
	DDRC |= 0x0F; /* first 4 pins in PORTC are now outputs */
	DDRA |= 0x3F; /* first 6 pins in PORTA are now outputs */

	TIMER1_Comp_On(); /* Timer 1 is now on. */
	INT0_On(); /* enable int0 */
	INT1_On(); /* enable int1 */
	INT2_On(); /* enable int2 */

	while(1)
	{
		/*
		 * the following 6 code paragraphs purpose is to keep all the 7 segments
		 * enabled without changing their value by:
		 * 1- changing the port value before enabling the specific 7 segment
		 * 2- enabling the specific 7 segment to read the value
		 * 3- turn off the 7 segment you just set to avoid any change in its
		 * value by changing the port value for other 7 segments
		 * 4- update the value of the port and turn on the specific 7 segment
		 * one more time.
		 */

		PORTC = ( PORTC & 0xF0 ) | ( sec0 & 0x0F );
		/* this line of code will display result in seconds */
		SET_BIT(PORTA,0); /* seconds 7 segment is enabled */
		CLEAR_BIT(PORTA,0); /* seconds 7 segment is disabled */

		PORTC = ( PORTC & 0xF0 ) | ( sec1 & 0x0F );
		/* this line of code will display result of seconds in tens form */
		SET_BIT(PORTA,1); /* tens of seconds 7 segment is enabled */
		CLEAR_BIT(PORTA,1); /* tens of seconds 7 segment is disabled */

		PORTC = ( PORTC & 0xF0 ) | ( min0 & 0x0F );
		/* this line of code will display result in minutes */
		SET_BIT(PORTA,2); /* minutes 7 segment is enabled */
		CLEAR_BIT(PORTA,2); /* minutes 7 segment is disabled */

		PORTC = ( PORTC & 0xF0 ) | ( min1 & 0x0F );
		/* this line of code will display result of minutes in tens form */
		SET_BIT(PORTA,3); /* tens of minutes 7 segment is enabled */
		CLEAR_BIT(PORTA,3); /* tens of minutes 7 segment is disabled */

		PORTC = ( PORTC & 0xF0 ) | ( hr0 & 0x0F );
		/* this line of code will display result in hours */
		SET_BIT(PORTA,4); /* hours 7 segment is enabled */
		CLEAR_BIT(PORTA,4); /* hours 7 segment is disabled */

		PORTC = ( PORTC & 0xF0 ) | ( hr1 & 0x0F );
		/* this line of code will display result of hours in tens form */
		SET_BIT(PORTA,5); /* tens of hours 7 segment is enabled */
		CLEAR_BIT(PORTA,5); /* tens of hours 7 segment is disabled */

		if (sec0 == SEVEN_SEGMENT_UPPER_LIMIT)
		{
			sec0 = 0; /* everytime sec0 reaches 9 it starts from 0 */
			sec1++; /* increment the tens value */
		}

		if (sec1 == SECONDS_UPPER_LIMIT)
		{
			sec1 = 0; /* when tens value reach maximum limit it starts from 0 */
			min0++; /* minutes will start counting after 60 seconds is reached */
		}
		if (min0 == SEVEN_SEGMENT_UPPER_LIMIT)
		{
			min0 = 0; /* everytime min0 reaches 9 it starts from 0 */
			min1++; /* increment the tens value */
		}
		if (min1 == MINUTES_UPPER_LIMIT)
		{
			min1 = 0; /* when tens value reach maximum limit it stars from 0 */
			hr0++; /* hours will start counting after 60 minutes is reached */
		}
		if (hr0 == SEVEN_SEGMENT_UPPER_LIMIT)
		{
			hr0 = 0; /* everytime hr0 reaches 9 it starts from 0 */
			hr1++; /* increment the tens value */
		}
		if (hr1 == HOURS1_UPPER_LIMIT && hr0 == HOURS0_UPPER_LIMIT)
		{
			/* StopWatch will start from the beginning when 24 hours is reached */
			sec0 = 0;
			sec1 = 0;
			min0 = 0;
			min1 = 0;
			hr0 = 0;
			hr1 = 0;
		}
	}
}
