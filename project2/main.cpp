#include "scheduler.h"
#include "LED_Test.h"
#include <stddef.h>

void taskA(){
	// turn on pin 13
	enableB(0b10000000);
	for (int i = 0; i < 32000; i++);
	disableB();
}

void taskB(){
	// turn on pin 12
	enableB(0b01000000);
	for (int i = 0; i < 32000; i++);
	disableB();
}

void idle(uint32_t idle_time)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.
	Scheduler_Dispatch_Oneshot(idle_time);
}

void setup()
{
	LinkedList<int> obj;
	Scheduler_Init();
	//start offset in ms, period in ms, function callback
	//Scheduler_StartPeriodicTask(0, 200, taskA, obj);
	//Scheduler_StartPeriodicTask(20, 200, taskB, obj);
}

void loop()
{
	uint32_t idle_time = Scheduler_Dispatch_Periodic();
	if (idle_time)
	{
		idle(idle_time);
	}
}

int main(){
	initB();
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}
