/*
 * control.c
 *
 *  Created on: 14-July-2010
 *      Author: lienh
 */
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "uart/uart.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();

uint8_t roomba_addr[5] = { 0x98, 0x76, 0x54, 0x32, 0x10 };	// roomba radio address

volatile uint8_t rxflag = 0;

//static radiopacket_t packet;
//pl4 and pl5 pan and tilt PF0 analog pin 0 lazer

int main()
{
	uint8_t i;
	clock8MHz();

	cli();

	// LEDs
	DDRL = 0xFF;
	PORTL = 0xFF;
	DDRB = 0xFF;
	uart_init(UART_9600, CH_2);//, CONTROL_UART_2);

	Roomba_Init(); // initialize the roomba

	sei();
	
	while(1){
		while (uart_bytes_received(CH_2) != 1);
		int byte1 = uart_get_byte(0, CH_2);
		if(byte1 == 2) PORTB = 0b10000000;
		else PORTB = 0b00000000;
	}

// 	UART test - drive straight forward at 100 mm/s for 0.5 second
	//Roomba_Drive(100, 0x8000);
			
	//_delay_ms(500);
		
	//Roomba_Drive(0, 0);
		


	return 0;
}

void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
	PORTL ^= _BV(PL7);
	_delay_ms(50);
	PORTL ^= _BV(PL7);
}
