#include "scheduler.h"
#include "LED_Test.h"
#include <stddef.h>
LinkedList<arg_t> obj;
LinkedList<arg_t> obj1;
int count = 0;

void taskC(LinkedList<arg_t> &obj){
	enableB(0b00010000);
	for (int i = 0; i < 32000; i++);
	disableB();
}

void taskA(LinkedList<arg_t> &obj){
	// turn on pin 13
	enableB(0b10000000);
	for (int i = 0; i < 32000; i++);
	count++;
	if (count == 4){
		count = 0;
		 Schedule_OneshotTask(10,10,taskC,1,obj1 );
		 //Schedule_OneshotTask(15,10,taskC,0,obj1 );
	}
	disableB();
}

void taskB(LinkedList<arg_t> &obj){
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
	Scheduler_Init();
	//start offset in ms, period in ms, function callback
	Scheduler_StartPeriodicTask(0, 200, taskA, obj);
	Scheduler_StartPeriodicTask(20, 200, taskB, obj);
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
	arg_t arg1{'a',1};
	obj1.push(arg1);
	initB();
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}
