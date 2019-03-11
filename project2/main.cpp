#include "scheduler.h"
#include "LED_Test.h"
#include <stddef.h>

void idle(uint32_t idle_time)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.
	Scheduler_Dispatch_Oneshot();
}

void setup()
{
	Scheduler_Init();
	//start offset in ms, period in ms, function callback
	
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
	initE();
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}
