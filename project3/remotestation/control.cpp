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
LinkedList<arg_t> servoList;
LinkedList<arg_t> driveList;
LinkedList<arg_t> roombaList;
LinkedList<arg_t> stateList;
LinkedList<arg_t> updateList;

//global variables
int speedPan = 0;
int speedTilt = 0;
int oldPanSpeed = 0;
int oldTiltSpeed = 0;
int velocityChange = 0;
int radiusChange = 0;
int oldVelocityChange = 0;
int oldVelocitySpeed = 0;
int ticksLeft = 9190; // available time for shooting

// send info
//		type and range
// state 0,					0 .. 1
// time to state change	1	0 .. 29
// lazer energy left 2		0 .. 100
// light sensor 3			0 .. 1
// DEATH 4					0 .. 1

// send over bluetooth by pairs
void write2bytes(uint8_t one, uint8_t two){
	uart_putchar(one, CH_2);
	uart_putchar(two, CH_2);
}


// lazer is pulsating consult with hamzah

bool lastpin = 0;
int lastShotTime = 0;
// not a periodic action
void lazerShot(LinkedList<arg_t> &obj){
	if(obj.front()->pin && !lastpin){
		// get current tic use that to see how long it's been and shut off after 2 seconds
		lastShotTime = get_time();
		PORTB = 0b01000000;
		lastpin = 1;
	}
	else if(lastpin){
		ticksLeft -= get_time() - lastShotTime;
		PORTB = 0b00000000;
		lastpin = 0;
	}
}

void servoMove(LinkedList<arg_t> &obj){
	
}

// polling task to change the roomba velocity
// occurs every 2 seconds
void roombaMove(LinkedList<arg_t> &obj){
	
}

// send state updates every 10 ms
int lastStateChange = 0;
int lastTicksleft = ticksLeft;
//update the states to the remote periodic task
void StateUpdate(LinkedList<arg_t> &obj){
	
	if(lastStateChange % 10 == 0){
		write2bytes(1, map(lastStateChange, 0, 300, 0, 29));
	}
	lastStateChange -=1;
	
	if(lastTicksleft != ticksLeft){
		write2bytes(2, map(ticksLeft, 0, 9190, 0, 100));
		lastTicksleft = ticksLeft;
	}
}

// 30 second periodic state change
void changeState(LinkedList<arg_t> &obj){
	if(obj.front()->pin){
		obj.front()->pin = 0;
		write2bytes(0,0);
	}
	else{
		obj.front()->pin = 1;
		write2bytes(0,1);
	}
	lastStateChange = 300;
}

// happens in the idle loop needs to be fast
void sampleInputs(){
	if(uart_bytes_received(CH_2) < 2) return;
	
	int v0 = uart_get_byte(0,CH_2);
	int v1 = uart_get_byte(1,CH_2);
	
	if(uart_bytes_received(CH_2) > 2) uart_set_front(2, CH_2);
	
	switch(v0){
		case(0)://lazer
		  switch(v1){
				case(0):
				LazerShotList.front()->pin = 0;
				Schedule_OneshotTask(10,10,lazerShot, 0, LazerShotList);
				break;
				case(1):
				LazerShotList.front()->pin = 1;
				Schedule_OneshotTask(10,10,lazerShot, 0, LazerShotList);
				break;
				}
			break;
				case(1):// pan
				speedPan = map(v1, 0, 81, -40, 40);
			break;
				case(2)://tilt
				speedTilt = map(v1, 0, 21, -10, 10);
			break;
				case(3):// velocity range -500 to 500
				velocityChange = map (v1, 0, 100, -50, 50); // making max change 50
			break; 			
				case(4):// radius range -2000 2000,
						//straight 32768 or hex 8000, spin in place -1 cw 1 ccw
				radiusChange = map(v1, 0, 100, -200, 200);
			break;
	}
	uart_reset_receive(CH_2);
	
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
	
	PWM_Init_Pan();
	PWM_Init_Tilt();
	sei();
	
// 	// 	UART test - drive straight forward at 100 mm/s for 0.5 second
// 	Roomba_Drive(100, 0x8000);
// 		
// 	_delay_ms(500);
// 				
// 	Roomba_Drive(0, 0);
	
	Scheduler_Init();
	
	// pins used as state identifiers
	arg_t lazer{'a', 0};
	arg_t state {'a', 1};
	
	// write default values to servos
	PWM_write_Pan(2050);
	PWM_write_Tilt(950);
	
	LazerShotList.push(lazer);
	stateList.push(state);
	adc_init();
	
	// start the schedules
	Scheduler_StartPeriodicTask(0, 27570, changeState, stateList); // 30 seconds
	Scheduler_StartPeriodicTask(10, 92, StateUpdate, updateList); // 0.1 seconds
	Scheduler_StartPeriodicTask(20, 1840, roombaMove, roombaList);  // 2 seconds
	Scheduler_StartPeriodicTask(30, 184, servoMove, servoList);  // 0.2 seconds
	
}


int main(){
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}