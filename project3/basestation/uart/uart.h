/**
 * @file   uart.c
 * @author Justin Tanner
 * @date   Sat Nov 22 21:32:03 2008
 *
 * @brief  UART Driver targetted for the AT90USB1287
 *
 */

#ifndef __UART_H__
#define __UART_H__

#include <avr/interrupt.h>
#include <stdint.h>

typedef enum _uart_bps
{
	UART_9600,
	UART_19200,
	UART_38400,
	UART_57600,
	UART_DEFAULT,
} UART_BPS;

typedef enum _uart_channel
{
	CH_1,
	CH_2,
} UART_CHANNEL;

#define UART_BUFFER_SIZE    32

void uart_init(UART_BPS bitrate, UART_CHANNEL);
void uart_putchar(uint8_t byte, UART_CHANNEL);
uint8_t uart_get_byte(int index, UART_CHANNEL);
uint8_t uart_bytes_received(UART_CHANNEL);
void uart_reset_receive(UART_CHANNEL);

#endif

