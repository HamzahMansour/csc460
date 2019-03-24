/* main.c
UVic CSC460 Spring 2019 Team 13 */

#include "uart/uart.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();

volatile uint8_t rxflag = 0;

int main()
{
	uint8_t i;
	clock8MHz();

	cli();

	// LEDs
	DDRL = 0xFF;
	PORTL = 0xFF;

	sei();

	return 0;
}