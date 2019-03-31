// /**
//  * @file   uart.c
//  * @author Justin Tanner
//  * @date   Sat Nov 22 21:32:03 2008
//  *
//  * @brief  UART Driver targetted for the AT90USB1287
//  *
//  */
// 
// #ifndef __UART_H__
// #define __UART_H__
// #include <stdint.h>
// #include <avr/io.h>
// #include <avr/interrupt.h>
// 
// typedef enum _uart_bps
// {
// 	UART_9600,
// 	UART_38400,
// 	UART_57600,
// 	UART_19200,
// 	UART_DEFAULT,
// } UART_BPS;
// 
// typedef enum _uart_setting
// {
// 	CONTROL_UART_1,
// 	CONTROL_UART_2,
// } UART_CONTROL;
// 
// #define UART_BUFFER_SIZE 32			// size of Rx ring buffer.
// 
// volatile uint8_t uart_rx1; 		// Flag to indicate uart received a byte
// volatile uint8_t uart_rx2; 
// 
// void uart_init(UART_BPS bitrate, UART_CONTROL controlsetting);
// void uart_putchar(char c, UART_CONTROL controlsetting);
// char uart_get_byte(int index, UART_CONTROL controlsetting);
// void uart_putstr(char *s, UART_CONTROL controlsetting);
// 
// uint8_t uart_bytes_received(UART_CONTROL controlsetting);
// void uart_reset_receive(UART_CONTROL controlsetting);
// //void uart_print(uint8_t* output, int size, UART_CONTROL controlsetting);
//#endif

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
#define F_CPU 8000000UL

typedef enum _uart_bps
{
	UART_9600,
	UART_19200,
	UART_38400,
	UART_57600,
	UART_DEFAULT,
} UART_BPS;

#define UART_BUFFER_SIZE    32

void uart_init(UART_BPS bitrate);
void uart_putchar(uint8_t byte);
uint8_t uart_get_byte(int index);
uint8_t uart_bytes_received(void);
void uart_reset_receive(void);

#endif

