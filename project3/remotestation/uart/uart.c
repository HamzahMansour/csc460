/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */
#include <avr/io.h>				
#include <avr/interrupt.h>		// ISR handling.
#include "uart.h"
#define F_CPU 16000000UL

#ifndef F_CPU
#warning "F_CPU not defined for uart.c."
#define F_CPU 11059200UL
#endif

/*
 Global Variables:
 Variables appearing in both ISR/Main are defined as 'volatile'.
*/
static volatile int rxn1; // buffer 'element' counter.
static volatile char rx1[UART_BUFFER_SIZE]; // buffer of 'char'.
static volatile int rxn2; // buffer 'element' counter.
static volatile char rx2[UART_BUFFER_SIZE]; // buffer of 'char'.

void uart_putchar (char c, UART_CONTROL cs)
{
	cli();
	switch(cs){
		case CONTROL_UART_1:
			while ( !( UCSR0A & (1<<UDRE0)) ); // Wait for empty transmit buffer
			UDR0 = c;  // Putting data into the buffer, forces transmission
			break;
		case CONTROL_UART_2:
			while ( !( UCSR2A & (1<<UDRE2)) ); // Wait for empty transmit buffer
			UDR2 = c;  // Putting data into the buffer, forces transmission
			break;
	}
	sei();
}

char uart_get_byte (int index, UART_CONTROL cs)
{
	switch(cs){
		case CONTROL_UART_1:
			if (index < UART_BUFFER_SIZE) {
				return rx1[index];
			}// Putting data into the buffer, forces transmission
			break;
		case CONTROL_UART_2:
			if (index < UART_BUFFER_SIZE) {
				return rx2[index];
			}// Putting data into the buffer, forces transmission
			break;
	}
	
	return 0;
}

void uart_putstr(char *s, UART_CONTROL cs)
{
	while(*s) uart_putchar(*s++, cs);
	
}

void uart_init(UART_BPS bitrate, UART_CONTROL cs){
	
	unsigned long baudprescale;

	/* Set baud rate */;
	switch (bitrate) {
	case UART_9600:
		baudprescale = 207;
		break;
    case UART_38400:
	    baudprescale = 51;
		break;
    case UART_57600:
        baudprescale = 34;
        break;
	case UART_19200:
		baudprescale = 103;
		break;
    default:
        baudprescale = 0;
    }
	
	switch(cs){
		case CONTROL_UART_1:
			rxn1 = 0;
			uart_rx1 = 0;
			UBRR0H = 0;
			UBRR0L = (uint8_t) baudprescale;

			/* Enable receiver and transmitter */
			UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0); 
		break;
		case CONTROL_UART_2:
			rxn2 = 0;
			uart_rx2 = 0;
			UBRR2H = 0;
			UBRR2L= (uint8_t) baudprescale;

			/* Enable receiver and transmitter */
			UCSR2B = (1<<RXEN2)|(1<<TXEN2)|(1<<RXCIE2); 
		break;
	}
	
	
}

uint8_t uart_bytes_received(UART_CONTROL cs)
{
	switch(cs){
		case CONTROL_UART_1:
		return rx1;
		break;
		case CONTROL_UART_2:
		return rx2;
		break;
	}
	return 0;
}

void uart_reset_receive(UART_CONTROL cs)
{
	switch(cs){
		case CONTROL_UART_1:
		rxn1 = 0;
		break;
		case CONTROL_UART_2:
		rxn2 = 0;
		break;
	}
}

/*
 Interrupt Service Routine (ISR):
*/

ISR(USART0_RX_vect)
{
	while ( !(UCSR0A & (1<<RXC0)) );

	//PORTB = ~_BV(PINB1);

	rx1[rxn1] = UDR0;
	rxn1 = (rxn1 + 1) % UART_BUFFER_SIZE;
	uart_rx1 = 1; // notify main of receipt of data.
	//PORTB = PORTB | _BV(PINB1);
}
/*
 Interrupt Service Routine (ISR):
*/

ISR(USART2_RX_vect)
{
	while ( !(UCSR2A & (1<<RXC2)) );

	//PORTB = ~_BV(PINB1);

	rx2[rxn2] = UDR2;
	rxn2 = (rxn2 + 1) % UART_BUFFER_SIZE;
	uart_rx2 = 1; // notify main of receipt of data.
	//PORTB = PORTB | _BV(PINB1);
}


/**
 * Prepares UART to receive another payload
 *
 */

void uart_print(uint8_t* output, int size, UART_CONTROL cs)
{
	uint8_t i;
	for (i = 0; i < size && output[i] != 0; i++)
	{
		uart_putchar(output[i], cs);
	}
}



