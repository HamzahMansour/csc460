#include "scheduler.h"
#include <avr/interrupt.h>
#include <stddef.h>
#include <string.h>
#include "LED_Test.h"
#include <stdlib.h>

#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)

typedef struct // period tasks
{
	int32_t period;
	int32_t remaining_time;
	uint8_t is_running;
	task_cb callback;
	LinkedList<arg_t> args;
} task_t;

task_t periodic_tasks[MAXTASKS];
LinkedList <oneshot_t> oneshot_tasks;
LinkedList <oneshot_t> system_tasks;

unsigned long current_tic;
uint32_t last_runtime;
uint32_t last_oneshottime = 0;
int remaining_Idletime = 0;
int idle_start = 0;
int task_type = 0;
int gidle_time = 0;
int time_conflict_count = 0;
int miss_system = 0;
int miss_oneshot = 0;

// setup our timer
void Scheduler_Init()
{
	current_tic = 0;
	Disable_Interrupt();
	//Clear timer config.
	TCCR1A = 0;
	TCCR1B = 0;
	//Set to CTC (mode 4)
	TCCR1B |= (1<<WGM12);
	
	TCCR1B |= (1<<CS10);
	TCCR1B |= (1<<CS12);
	
	//Set TOP value 10ms
	OCR1A = 16;
	
	//Enable interupt A for timer 3.
	TIMSK1 |= (1<<OCIE1A);
	
	//Set timer to 0 (optional here).
	TCNT1 = 0;
	Enable_Interrupt();
	
}

ISR(TIMER1_COMPA_vect){
	// use the timer to determine the time
	current_tic++;
	if(idle_start + gidle_time < current_tic){
		if(task_type >= 2) {
			disableB();
			disableE();
			exit(EXIT_FAILURE); // critical failure
		}
	}
	if(time_conflict_count > 10){
		disableB();
		disableE();
		enableE(0b00010000); // pin 2
		disableE();
		exit(EXIT_FAILURE); // critical failure
	}
}

void Scheduler_StartPeriodicTask(int16_t delay, int16_t period, task_cb task, LinkedList<arg_t> args)
{
	static uint8_t id = 0;
	if (id < MAXTASKS)
	{
		periodic_tasks[id].remaining_time = delay;
		periodic_tasks[id].period = period;
		periodic_tasks[id].is_running = 1;
		periodic_tasks[id].callback = task;
		periodic_tasks[id].args = args;
		id++;
	}
}

void Schedule_OneshotTask(int32_t remaining_time,	int32_t max_time, task_cb callback,	int priority,	LinkedList<arg_t> args){
	static uint32_t sid = 0;
	static uint32_t oid = 0;
	if (priority)
	{
		oneshot_t newtask = {
			remaining_time, max_time, 0, callback, priority, sid, args,
		};
		sid++;
		system_tasks.push(newtask);
	}
	else {
		oneshot_t newtask = {
			remaining_time, max_time, 0, callback, priority,oid, args,
		};
		oid++;
		oneshot_tasks.push(newtask);
		
	}
}

uint32_t min(uint32_t param1, uint32_t param2)
{
	if(param1 < param2)return param1;
	
	return param2;
}

uint32_t Scheduler_Dispatch_Periodic()
{
	uint8_t i;

	uint32_t now = current_tic;
	uint32_t elapsed = now - last_runtime;
	last_runtime = current_tic;
	task_cb t = NULL;
	uint32_t idle_time = 0xFFFFFFFF;
	LinkedList<arg_t> args;
	int task_indx = -1;
	
	
	// update each task's remaining time, and identify the first ready task (if there is one).
	for (i = 0; i < MAXTASKS; i++){
		if (periodic_tasks[i].is_running){
			// update the task's remaining time
			periodic_tasks[i].remaining_time -= elapsed;
			if (periodic_tasks[i].remaining_time <= 0){
				if (periodic_tasks[i].remaining_time < 0){
					time_conflict_count++;
				}
				
				if (t == NULL){
					
					// if this task is ready to run, and we haven't already selected a task to run,
					// select this one.
					task_indx = i;
					t = periodic_tasks[i].callback;
					args = periodic_tasks[i].args;
				}
				idle_time = 0;
			}
			else {
				idle_time = min((uint32_t)periodic_tasks[i].remaining_time, idle_time);
			}
		}
	}
	if (t != NULL){
		if(!system_tasks.empty()) miss_system++;
		if(!oneshot_tasks.empty()) miss_oneshot++;
		
		// If a task was selected to run, call its function.
		task_type = 1;
		t(args);
		task_type = 0;
		periodic_tasks[task_indx].remaining_time = periodic_tasks[task_indx].period;
	}
	
	last_oneshottime = current_tic;
	remaining_Idletime = idle_time;
	idle_start = current_tic;
	gidle_time = idle_time;
	return idle_time;
}

void Scheduler_Dispatch_Oneshot(){
	// check our error conditions
	// missed too many periods
	if(miss_system > 10){
		disableB();
		disableE();
		enableE(0b00100000); // pin 3
		disableE();
		exit(EXIT_FAILURE); // critical failure
	}
	if(miss_oneshot > 5){
		oneshot_tasks.pop();
	}

	oneshot_t next_task;
	bool nexttask_allocated = false;
	int elapsedOneshots = current_tic - last_oneshottime;
	last_oneshottime = current_tic;
	remaining_Idletime -= elapsedOneshots;

	if(!system_tasks.empty()){
		if(system_tasks.front()->max_time < remaining_Idletime){
			next_task = *system_tasks.front();
			nexttask_allocated  = true;
		}
		} else if(!oneshot_tasks.empty()){
		if(oneshot_tasks.front()->max_time < remaining_Idletime){
			next_task = *oneshot_tasks.front();
			nexttask_allocated = true;
		}
	}
	if(nexttask_allocated) Scheduler_RunTask_Oneshot(next_task);
}

void Scheduler_RunTask_Oneshot(oneshot_t next_task){

	// run the task
	next_task.is_running = 1;
	if(next_task.priority)
	task_type = 2;
	else
	task_type = 3;
	next_task.callback(next_task.args);
	task_type = 0;
	//

	// --- task is running, could be interrupted?

	// task is done running:
	if(next_task.priority){
			if(!system_tasks.empty()) {
				system_tasks.pop();
				miss_system = 0;
			}
		} else {
			if(!oneshot_tasks.empty()) {
				oneshot_tasks.pop();
				miss_oneshot = 0;
			}
	    }
}
