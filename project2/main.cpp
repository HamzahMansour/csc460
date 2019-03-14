#include "scheduler.h"
#include "LED_Test.h"
#include <stddef.h>
LinkedList<arg_t> obj;
LinkedList<arg_t> obj1;
int count = 0;

void taskC(LinkedList<arg_t> &obj){
	enableB(0b00100000); // pin 11
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void taskD(LinkedList<arg_t> &obj){
	enableB(0b00010000); // pin 10
	int t;
	for (int i = 0; i < 32000; i++) t++;
	Schedule_OneshotTask(10,10,taskC,0,obj1 );
	Schedule_OneshotTask(15,20,taskC,0,obj1 );
	disableB();
}

void taskD2(LinkedList<arg_t> &obj){
	enableB(0b00010000); // pin 10
	int t;
	for (int i = 0; i < 320000; i++) t++;
	Schedule_OneshotTask(10,60,taskC,0,obj1 );
	Schedule_OneshotTask(15,60,taskC,0,obj1 );
	disableB();
}

void taskA0(LinkedList<arg_t> &obj){
	enableB(0b10000000); // pin 13
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void taskA(LinkedList<arg_t> &obj){
	enableB(0b10000000); // pin 13
	int t;
	for (int i = 0; i < 32000; i++) t++;
	count++;
	if (count == 4){
		count = 0;
		Schedule_OneshotTask(15,60,taskD,0,obj1 );
		Schedule_OneshotTask(10,60,taskC,1,obj1 );
		Schedule_OneshotTask(15,60,taskC,0,obj1 );
	}
	disableB();
}

void taskA2(LinkedList<arg_t> &obj){
	enableB(0b10000000); // pin 13
	int t;
	for (int i = 0; i < 32000; i++) t++;
	count++;
	if (count == 4){
		count = 0;
		Schedule_OneshotTask(15,60,taskD2,0,obj1 );
		Schedule_OneshotTask(10,60,taskC,1,obj1 );
		Schedule_OneshotTask(15,60,taskC,0,obj1 );
	}
	disableB();
}

void taskA3(LinkedList<arg_t> &obj){
	enableB(0b10000000); // pin 13
	int t;
	for (int i = 0; i < 32000; i++) t++;
	count++;
	if (count == 4){
		count = 0;
		Schedule_OneshotTask(15,60,taskD,0,obj1 );
		Schedule_OneshotTask(10,300,taskC,1,obj1 );
		Schedule_OneshotTask(15,60,taskC,0,obj1 );
	}
	disableB();
}

void taskA4(LinkedList<arg_t> &obj){
	enableB(0b10000000); // pin 13
	int t;
	for (int i = 0; i < 32000; i++) t++;
	count++;
	if (count == 4){
		count = 0;
		Schedule_OneshotTask(15,60,taskD,0,obj1 );
		Schedule_OneshotTask(10,60,taskC,1,obj1 );
		Schedule_OneshotTask(15,400,taskC,0,obj1 );
	}
	disableB();
}

void taskB(LinkedList<arg_t> &obj){
	enableB(0b01000000); // pin 12
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void test_success(){
	Scheduler_StartPeriodicTask(0, 400, taskA, obj);
	Scheduler_StartPeriodicTask(70, 400, taskB, obj);
}

void test_toolong(){
	Scheduler_StartPeriodicTask(0, 400, taskA2, obj);
	Scheduler_StartPeriodicTask(0, 400, taskB, obj);
}

void test_misssystem(){
	Scheduler_StartPeriodicTask(0, 400, taskA3, obj);
	Scheduler_StartPeriodicTask(0, 400, taskB, obj);
}

void test_missoneshot(){
	Scheduler_StartPeriodicTask(0, 400, taskA4, obj);
	Scheduler_StartPeriodicTask(0, 400, taskB, obj);
}

void test_impossibleschedule(){
	Scheduler_StartPeriodicTask(0, 20, taskA0, obj);
	Scheduler_StartPeriodicTask(0, 40, taskB, obj);
}

void test_timeconflict(){
	Scheduler_StartPeriodicTask(0, 400, taskA, obj);
	Scheduler_StartPeriodicTask(0, 400, taskB, obj);
}

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
	test_success();
//	test_toolong();
//	test_misssystem();
//	test_missoneshot();
//	test_impossibleschedule();
//	test_timeconflict();

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
	initE();
	setup();
	for (;;){
		loop();
	}
	for (;;);
	return 0;
}
