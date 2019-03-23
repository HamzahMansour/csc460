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
#define F_CPU 8000000UL

#ifndef F_CPU
#warning "F_CPU not defined for uart.c."
#define F_CPU 11059200UL
#endif

/*
 Global Variables:
 Variables appearing in both ISR/Main are defined as 'volatile'.
*/
static volatile int rxn; // buffer 'element' counter.
static volatile char rx[UART_BUFFER_SIZE]; // buffer of 'char'.

void uart_putchar (char c)
{
	cli();
	while ( !( UCSR1A & (1<<UDRE1)) ); // Wait for empty transmit buffer           
	UDR1 = c;  // Putting data into the buffer, forces transmission
	sei(); // may want to replace with sreg
}

char uart_get_byte (int index)
{
	if (index < UART_BUFFER_SIZE) {
		return rx[index];
	}
	return 0;
}

void uart_putstr(char *s)
{
	while(*s) uart_putchar(*s++);
	
}

void uart_init(UART_BPS bitrate){

	//uint8_t sreg = SREG;
	//cli();
	
	// Make sure I/O clock to USART1 is enabled
	PRR1 &= ~(1 << PRUSART1);
	
	// Set baud rate to 19.2k at fOSC = 16 MHz
	
	switch(bitrate) {
		case UART_19200:
		UBRR1 = 51;
		break;
		case UART_38400:
		UBRR1 = 25;
		break;
		case UART_57600:
		UBRR1 = 16;
		break;
		case UART_115200:
		UBRR1 = 8;
		break;
		default:
		UBRR1 = 16;
	}
	
	// Clear USART Transmit complete flag, normal USART transmission speed
	UCSR1A = (1 << TXC1) | (0 << U2X1);
	
	// Enable receiver, transmitter, and rx complete interrupt.
	UCSR1B = (1<<RXEN1)|(1<<TXEN1)|(1<<RXCIE1);
	// 8-bit data
	UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));
	// disable 2x speed
	UCSR1A &= ~(1<<U2X1);
	// SREG = reg
}

uint8_t uart_bytes_received(void)
{
	return rxn;
}

void uart_reset_receive(void)
{
	rxn = 0;
}

/*
 Interrupt Service Routine (ISR):
*/

ISR(USART1_RX_vect)
{

	//PORTB = ~_BV(PINB1);

	rx[rxn] = UDR1;
	rxn = (rxn + 1) % UART_BUFFER_SIZE;
	uart_rx = 1; // notify main of receipt of data.
	//PORTB = PORTB | _BV(PINB1);
}

/**
 * Prepares UART to receive another payload
 *
 */

void uart_print(uint8_t* output, int size)
{
	uint8_t i;
	for (i = 0; i < size && output[i] != 0; i++)
	{
		uart_putchar(output[i]);
	}
}



