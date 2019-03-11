#include <avr/io.h>
#include "LED_Test.h"
#include "scheduler.h"
LinkedList<arg_t> obj;
LinkedList<arg_t> obj1;
int count = 0;


void taskC(LinkedList<arg_t> &obj){
	enableB(0b00010000);
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void taskDmultispawn(LinkedList<arg_t> &obj){
	enableB(0b00100000);
	int t;
	for (int i = 0; i < 320000; i++) t++;
	Schedule_OneshotTask(10,10,taskC,0,obj1 );
	Schedule_OneshotTask(15,20,taskC,0,obj1 );
	disableB();
}

void taskDunispawn(LinkedList<arg_t> &obj){
	enableB(0b00100000);
	int t;
	for (int i = 0; i < 320000; i++) t++;
	Schedule_OneshotTask(10,10,taskC,0,obj1 );
	Schedule_OneshotTask(15,20,taskC,0,obj1 );
	disableB();
}

void taskAmultispawn(LinkedList<arg_t> &obj){
	// turn on pin 13
	enableB(0b10000000);
	int t;
	for (int i = 0; i < 32000; i++) t++;
	count++;
	if (count == 4){
		count = 0;
		Schedule_OneshotTask(15,300,taskD,0,obj1 );
		Schedule_OneshotTask(10,20,taskC,1,obj1 );
		Schedule_OneshotTask(15,20,taskC,0,obj1 );
	}
	disableB();
}

void taskAunispawn(LinkedList<arg_t> &obj){
	// turn on pin 13
	enableB(0b10000000);
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void taskB(LinkedList<arg_t> &obj){
	// turn on pin 12
	enableB(0b01000000);
	int t;
	for (int i = 0; i < 32000; i++) t++;
	disableB();
}

void initB(){
	DDRB = 0xFF;
	PORTB = 0x00;
}

void disableB(){
	PORTB = 0x00;
}

void enableB(unsigned int mask){
	PORTB = mask;
}

void initE(){
	DDRE = 0xFF;
	PORTE = 0x00;
}

void disableE(){
	PORTE = 0x00;
}

void enableE(unsigned int mask){
	PORTE = mask;
}

void test_workingSmoothly(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
	
	
}

void test_taskClash(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
	
}

void test_impossiblePeriod(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
}

void test_deceptiveOneshot(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
	
}

void test_tooLongSystemTask(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
	
}

void test_tooLongOneshotTask(){
	arg_t arg1{'a',1};
	obj1.push(arg1);
	
}

