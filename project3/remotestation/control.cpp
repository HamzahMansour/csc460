/*
 * control.c
 *
 *  Created on: 14-July-2010
 *      Author: lienh
 */
#include "roomba/roomba.h"
#include "roomba/roomba_sci.h"
#include "uart/uart.h"
#include "TTA/scheduler.h"
#include "util/AVRUtilities.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();
// global argument lists
LinkedList<arg_t> LazerShotList;
LinkedList<arg_t> temp;
bool lazer = 0;


// not a periodic action
void lazerShot(LinkedList<arg_t> &obj){
	if(obj.front()->pin){
		
		PORTB = 0b10000000;
	}
	else{
		PORTB = 0b00000000;
	}
}

// happens in the idle loop needs to be fast
void sampleInputs(){
	if(uart_bytes_received(CH_2) < 2) return;
	int i = 0;
	for(; i + 1 < uart_bytes_received(CH_2); i += 2){
		int v0 = uart_get_byte(i,CH_2);
		int v1 = uart_get_byte(i+1, CH_2);
		
		switch(v0){
			case(0)://lazer
			  switch(v1){
				  case(0):
					LazerShotList.front()->pin = 0;
					break;
				   case(1):
					LazerShotList.front()->pin = 1;
					break;
			  }
			break;
			case(1):
			break;
		}
	}
	
}

//static radiopacket_t packet;
//pl4 and pl5 pan and tilt PF0 analog pin 0 lazer

void idle(uint32_t idle_time)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.
	Scheduler_Dispatch_Oneshot();
	sampleInputs();
}


void loop()
{
	uint32_t idle_time = Scheduler_Dispatch_Periodic();
	if (idle_time)
	{
		idle(idle_time);
	}
}

void setup()
{
	//start offset in ms, period in ms, function callback
	clock8MHz();

	cli();

	// LEDs
	DDRL = 0xFF;
	PORTL = 0xFF;
	DDRB = 0xFF;
	uart_init(UART_9600, CH_2);

	Roomba_Init(); // initialize the roomba

	sei();
	
	// 	UART test - drive straight forward at 100 mm/s for 0.5 second
	Roomba_Drive(100, 0x8000);
		
	_delay_ms(500);
				
	Roomba_Drive(0, 0);
	
	Scheduler_Init();
	
	arg_t lazer{'a', 0};
	LazerShotList.push(lazer);
	
	//Scheduler_StartPeriodicTask(10, 200, periodicTest, temp);
	
}


int main(){
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}