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
LinkedList<arg_t> roombaList;
LinkedList<arg_t> stateList;
LinkedList<arg_t> updateList;
LinkedList<arg_t> resetList;

//global variables
int speedPan = 0;
int speedTilt = 0;
int oldPanSpeed = 0;
int oldTiltSpeed = 0;
int pan = 2050;
int tilt = 950;
int velocity = 0;
int velocityChange = 0;
int radiusChange = 0;
int oldVelocityChange = 0;
int oldradiusChange = 0;
int ticksLeft = 9190; // available time for shooting
const float DARK_THRESHOLD = 250000.0;//300000.0;

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
unsigned long lastShotTime = 0;
// not a periodic action
void lazerShot(LinkedList<arg_t> &obj){
	if(!lastpin && obj.front()->pin){
		lastpin = 1;
		PORTB |= 0b01000000;
		Roomba_PlaySong(3);
		// get current tic use that to see how long it's been and shut off after 2 seconds
		lastShotTime = get_time();
	}
	else if(lastpin && !obj.front()->pin){
		ticksLeft -= get_time() - lastShotTime;
		PORTB ^= 0b01000000;
		lastpin = 0;
	}
}

void speedCalculation(int* position, int* newVal, int* oldVal, int max_adjust, int topVal, int bottomVal){
	int tempt = *newVal;
	if( *oldVal > tempt && *oldVal - tempt > max_adjust){
		tempt = *oldVal - max_adjust;
	}
	else if(*oldVal < tempt && *oldVal - tempt < -max_adjust){
		tempt = *oldVal + max_adjust;
	}
	if(*position + tempt > topVal || *position + tempt < bottomVal){
		tempt = 0;
	}
	*oldVal = tempt;
	if(tempt != 0){
		*position += tempt;
		if(*position >= topVal)
			*position = topVal;
		if(*position <= bottomVal)
			*position = bottomVal;
	}
}

void servoMove(LinkedList<arg_t> &obj){
	//calculate pan and tilt
	speedCalculation(&pan, &speedPan, &oldPanSpeed, 5, 2100, 750);
	speedCalculation(&tilt, &speedTilt, &oldTiltSpeed, 2, 1000, 500);
	
	PWM_write_Pan(pan);
	PWM_write_Tilt(tilt);
}

// polling task to change the roomba velocity
// occurs every 2 seconds
// ignore commands when a flag is set
int oldDanger = 0;
bool backup = 0;
void roombaMove(LinkedList<arg_t> &obj){
	// calculate velocity and radius
	if(backup){
		return;
	}
	int tempR = radiusChange;
	int tempV = velocityChange;
	
	//if(stateList.front()->pin == 0) tempV = 0; // in stand-still mode, only spin in place

	// going straight
	if(tempR == 0){
		if(tempV == 0) tempR = 0;
		else tempR = 0x8000;
	} // spinning in place
	else if(tempV == 0){
		if (tempR > 0){
			tempR = 1;
			tempV = map(radiusChange, 1, 2000, 300, 1);
		}
		if (tempR < 0){
			tempR = -1;
			tempV = map(radiusChange, -1, -2000, 300, 1);
		}
	}
	
	Roomba_Drive(tempV, tempR);
}

// send state updates every 10 ms
int lastStateChange = 0;
int lastTicksleft = ticksLeft;
int lastLightOn = 0;
bool danger = 0;
//update the states to the remote periodic task
void StateUpdate(LinkedList<arg_t> &obj){
	
	if(((get_time() - lastShotTime) >= 1900 && lastpin) 
		|| (ticksLeft <= 0 && lastpin)){
		LazerShotList.front()->pin = 0;
		Schedule_OneshotTask(10,10,lazerShot, 0, LazerShotList);
	}
	
	if((get_time() - lastLightOn) >= 1838 && danger){
		Roomba_Drive(0,0);
		write2bytes(4,1);
		Roomba_PlaySong(0);
		// may need to wait
		_delay_ms(5000);
		Roomba_ChangeState(PASSIVE_MODE);
		exit(EXIT_SUCCESS);
	}
	
	if(lastStateChange % 10 == 0){
		write2bytes(1, map(lastStateChange, 0, 300, 0, 29));
	}
	lastStateChange -=1;
	
	if(lastTicksleft != ticksLeft){
		write2bytes(2, map(ticksLeft, 0, 9190, 0, 100));
		lastTicksleft = ticksLeft;
	}
	
	if(danger && !oldDanger){
		write2bytes(3,1);
	}
	else if(!danger && oldDanger){
		write2bytes(3,0);
	}
	oldDanger = danger;
	
	//may be too fast
	roomba_sensor_data_t data;
	Roomba_UpdateSensorPacket(EXTERNAL, &data);
	bool senseWall = (Roomba_BumperActivated(&data) || data.wall || data.virtual_wall);
	if(senseWall && !backup){
		Roomba_Drive(-200, 0x8000);
		backup = 1;
	}
	else if(!senseWall && backup){
		backup = 0;
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

void reset(LinkedList<arg_t> &obj){
	uart_reset_receive(CH_2);
}


// happens in the idle loop needs to be fast
void sampleInputs(){
	if(uart_bytes_received(CH_2) < 2) return;
	
	int v0 = uart_get_byte(0,CH_2);
	int v1 = uart_get_byte(1,CH_2);
	
	switch(v0){
		case(0)://lazer
		switch(v1){
			case(0):
			LazerShotList.front()->pin = 0;
			if(ticksLeft > 0) Schedule_OneshotTask(10,10,lazerShot, 0, LazerShotList);
			break;
			case(1):
			LazerShotList.front()->pin = 1;
			if(ticksLeft > 0) Schedule_OneshotTask(10,10,lazerShot, 0, LazerShotList);
			break;
		}
		break;
		case(1):// pan
		speedPan = map(v1, 0, 81, 80, -80);
		break;
		case(2)://tilt
		speedTilt = map(v1, 0, 21, -20, 20);
		break;
		case(3):// velocity range -500 to 500
		velocityChange = map (v1, 0, 100, -300, 300); // making max change 50
		break;
		case(4):// radius range -2000 2000,
		//straight 32768 or hex 8000, spin in place -1 cw 1 ccw
		if(v1 < 50) radiusChange = map(v1, 0, 49, 1, 2000);
		else if(v1 > 50) radiusChange = map(v1, 51, 101, -2000, -1);
		else if(v1 == 50) radiusChange = 0;
		break;
	}
	if(uart_bytes_received(CH_2) > 2)
	uart_set_front(2, CH_2);
	else
	uart_reset_receive(CH_2);
	
}

void read_LS(){
	int LSVal = read_adc(15);
	
	float lightV = LSVal * 4.98 / 1023.0;
	float lightR = 5300.0 * (4.98 / lightV - 1.0);
	 // If resistance of photocell is greater than the dark
	 if(lightR >= DARK_THRESHOLD){
		 danger = 0;
	 }
	 else{
		 if(!danger) lastLightOn = get_time();
		 danger = 1;
	 }
}

void idle(uint32_t idle_time)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.
	Scheduler_Dispatch_Oneshot();
	sampleInputs();
	read_LS();
}


void loop()
{
	uint32_t idle_time = Scheduler_Dispatch_Periodic();
	if (idle_time)
	{
		idle(idle_time);
	}
}

// 6 is nonce
void setup()
{
	//start offset in ms, period in ms, function callback
	clock8MHz();

	cli();

	// LEDs
	DDRL = 0xFF;
	PORTL = 0xFF;
	DDRB = 0xFF;
	
	//light sensor
	PORTA = 0xFF;
	
	// TX2 RX2
	uart_init(UART_9600, CH_2);
	
	//dd is digital 32, TX1 RX1
	Roomba_Init(); // initialize the roomba
	
	// pin3
	PWM_Init_Pan();
	// pin8
	PWM_Init_Tilt();
	
// 		UART test - drive straight forward at 100 mm/s for 0.5 second
// 		Roomba_Drive(100, 0x8000);
// 			
// 		_delay_ms(500);
// 					
// 		Roomba_Drive(0, 0);
	
	sei();
	
	// load my songs
	uint8_t death_notes[5] = {94, 93, 92, 91, 90};
	uint8_t death_durations[5] = {32, 32, 32, 32, 64};
		
	//uint8_t laser_notes[9] = {115, 119, 122, 125, 127, 125, 122, 119, 115};
	//uint8_t laser_durations[9] = {4, 4, 4, 4, 4, 4, 4, 4, 4};
	Roomba_LoadSong(0, death_notes, death_durations, 5);
	//Roomba_LoadSong(3, laser_notes, laser_durations, 9);
	
	//Roomba_PlaySong(3);
	
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
	Scheduler_StartPeriodicTask(21, 460, roombaMove, roombaList);  // 0.5 seconds
	Scheduler_StartPeriodicTask(15, 46, servoMove, servoList);	// 0.05 seconds
	Scheduler_StartPeriodicTask(30, 1838, reset, resetList);	// 2 seconds
	
}


int main(){
	setup();
 	for (;;){
 		loop();
 	}
	for (;;);
	return 0;
}