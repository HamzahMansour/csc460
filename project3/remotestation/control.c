/*
 * control.c
 *
 *  Created on: 14-July-2010
 *      Author: lienh
 */
#include "radio/radio.h"
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "uart/uart.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();

uint8_t roomba_addr[5] = { 0x98, 0x76, 0x54, 0x32, 0x10 };	// roomba radio address

volatile uint8_t rxflag = 0;

static radiopacket_t packet;

int main()
{
	uint8_t i;
	clock8MHz();

	cli();

	// LEDs
	DDRL = 0xFF;
	PORTL = 0xFF;

	Roomba_Init(); // initialize the roomba

	Radio_Init();
	Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
	Radio_Configure_Rx(RADIO_PIPE_0, roomba_addr, ENABLE); // setting the receving address

	sei();

	// UART test - drive straight forward at 100 mm/s for 0.5 second
	/*
	Roomba_Drive(100, 0x8000);

	_delay_ms(500);

	Roomba_Drive(0, 0);
	*/

	for (;;)
	{
		if (rxflag)
		{
			_delay_ms(20);

			// Copy the received packet into the radio packet structure.  If there are no more packets,
			// then clear the rxflag so that the interrupt will set it next time a packet is received.
			if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS)
			{
				rxflag = 0;
			}

			// If the packet is not a command, blink an error and don't do anything.
			if (packet.type != COMMAND)
			{
				PORTL ^= _BV(PL7);
				continue;
			}

			if (packet.payload.command.command == START ||
					packet.payload.command.command == BAUD ||
					packet.payload.command.command == CONTROL ||
					packet.payload.command.command == SAFE ||
					packet.payload.command.command == FULL ||
					packet.payload.command.command == SENSORS)
			{
				// Don't pass the listed commands to the Roomba.
				continue;
			}

			// Output the command to the Roomba, followed by its arguments.
			uart_putchar(packet.payload.command.command);
			for (i = 0; i < packet.payload.command.num_arg_bytes; i++)
			{
				uart_putchar(packet.payload.command.arguments[i]);
			}

			// Set the radio's destination address to be the remote station's address
			Radio_Set_Tx_Addr(packet.payload.command.sender_address);

			// Update the Roomba sensors into the packet structure that will be transmitted.
			Roomba_UpdateSensorPacket(1, &packet.payload.sensors.sensors);
			Roomba_UpdateSensorPacket(2, &packet.payload.sensors.sensors);
			Roomba_UpdateSensorPacket(3, &packet.payload.sensors.sensors);

			// send the sensor packet back to the remote station.
			packet.type = SENSOR_DATA;

			if (Radio_Transmit(&packet, RADIO_WAIT_FOR_TX) == RADIO_TX_MAX_RT)
			{
				PORTL ^= _BV(PL4);	// flash if the packet was dropped
				_delay_ms(10);
				PORTL ^= _BV(PL4);
			}
			else
			{
				PORTL ^= _BV(PL5);	// flash if the packet was received correctly
				_delay_ms(10);
				PORTL ^= _BV(PL4);
			}
		}
		else {
			// stand by
			PORTL ^= _BV(PL0);
			_delay_ms(10);
		}
	}

	return 0;
}

void radio_rxhandler(uint8_t pipenumber)
{
	rxflag = 1;
	PORTL ^= _BV(PL7);
	_delay_ms(50);
	PORTL ^= _BV(PL7);
}
