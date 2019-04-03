/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 *
 */
#include "uart.h"

#ifndef F_CPU
#warning "F_CPU not defined for uart.c."
#define F_CPU 11059200UL
#endif

static volatile uint8_t uart_buffer_1[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index_1;
static volatile uint8_t uart_buffer_2[UART_BUFFER_SIZE];
static volatile uint8_t uart_buffer_index_2;
/**
 * Initalize UART
 *
 */

void uart_init(UART_BPS bitrate, UART_CHANNEL cs)
{
	int rate;
	switch (bitrate)
	{
		case UART_9600:
		rate = 207;
		break;
		case UART_19200:
		rate = 103;
		break;
		case UART_38400:
		rate = 51;
		break;
		default:
		rate = 103;
		break;
	}
	switch(cs){
		case CH_1:
			UBRR1H = 0;
			UBRR1L = rate;
		
			// Clear USART Transmit complete flag, normal USART transmission speed
			UCSR1A = _BV(U2X1);

			// Enable receiver, transmitter, rx complete interrupt and tx complete interrupt.
			UCSR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);

			// 8-bit data
			UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
		
			uart_buffer_index_1 = 0;
		break;
		case CH_2:
			uart_buffer_index_2 = 0;
			UBRR2H = 0;
			UBRR2L = rate;
			// Clear USART Transmit complete flag, normal USART transmission speed
			UCSR2A = _BV(U2X2);

			// Enable receiver, transmitter, rx complete interrupt and tx complete interrupt.
			UCSR2B = _BV(RXEN2) | _BV(TXEN2) | _BV(RXCIE2);

			// 8-bit data
			UCSR2C = _BV(UCSZ21) | _BV(UCSZ20);

		break;
	}
}

/**
 * Transmit one byte
 * NOTE: This function uses busy waiting
 *
 * @param byte data to trasmit
 */
void uart_putchar(uint8_t byte, UART_CHANNEL cs)
{
	switch(cs){
		case(CH_1):
			/* wait for empty transmit buffer */
			while (!( UCSR1A & (1 << UDRE1)));

			/* Put data into buffer, sends the data */
			UDR1 = byte;
		break;
		case(CH_2):
			/* wait for empty transmit buffer */
			while (!( UCSR2A & (1 << UDRE2)));

			/* Put data into buffer, sends the data */
			UDR2 = byte;
		break;
	}
    
}

/**
 * Receive a single byte from the receive buffer
 *
 * @param index
 *
 * @return
 */
uint8_t uart_get_byte(int index, UART_CHANNEL cs)
{
	switch(cs){
		case(CH_1):
		if (index < UART_BUFFER_SIZE)
		{
			return uart_buffer_1[index];
		}
		break;
		case(CH_2):
		if(index < UART_BUFFER_SIZE)
		{
			return uart_buffer_2[index];
		}
		break;
	}
	
    return 0;
}

/**
 * Get the number of bytes received on UART
 *
 * @return number of bytes received on UART
 */
uint8_t uart_bytes_received(UART_CHANNEL cs)
{
	switch(cs){
		case(CH_1):
		return uart_buffer_index_1;
		break;
		case(CH_2):
		return uart_buffer_index_2;
		break;
	}
	return 0;
}

/**
 * Prepares UART to receive another payload
 *
 */
void uart_reset_receive(UART_CHANNEL cs)
{
	switch(cs){
		case(CH_1):
		uart_buffer_index_1 = 0;
		break;
		case(CH_2):
		uart_buffer_index_2 = 0;
		break;
	}
}

void uart_set_front(int index, UART_CHANNEL cs){
	switch(cs){
		case(CH_1):
		uart_buffer_1[0] = uart_buffer_1[index - 1];
		uart_buffer_index_1 = 1;
		break;
		case(CH_2):
		uart_buffer_2[0] = uart_buffer_2[index - 1];
		uart_buffer_index_2 = 1;
		break;
	}
}

/**
 * UART receive byte ISR
 */
ISR(USART1_RX_vect)
{
	while(!(UCSR1A & (1<<RXC1)));
    uart_buffer_1[uart_buffer_index_1] = UDR1;
    uart_buffer_index_1 = (uart_buffer_index_1 + 1) % UART_BUFFER_SIZE;
}

/**
 * UART receive byte ISR
 */
ISR(USART2_RX_vect)
{
	while(!(UCSR2A & (1<<RXC2)));
    uart_buffer_2[uart_buffer_index_2] = UDR2;
    uart_buffer_index_2 = (uart_buffer_index_2 + 1) % UART_BUFFER_SIZE;
}





