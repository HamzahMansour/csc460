#include "scheduler.h"
#include <avr/interrupt.h>
#include <stddef.h>
#include <string.h>


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

// setup our timer
void Scheduler_Init()
{
	current_tic = 0;
	Disable_Interrupt();
	TCCR3A = 0;
	TCCR3B = 0;
	// set to CTC (mode 4)
	TCCR3B != (1<<WGM32);
	// set prescaller to 256
	TCCR3B |= (1<<CS32);
	
	OCR3A = 312; // about 5 ms
	
	TIMSK3 |= (1<<OCIE3A);
	Enable_Interrupt();
	
}

ISR(TIMER3_COMPA_vect){
	// use the timer to determine the time
	current_tic++;
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

void Schedule_OneshotTask(int32_t remaining_time,	int32_t max_time,	uint8_t is_running,	task_cb callback,	int priority,	LinkedList<arg_t> args){
	
	if (priority)
	{
		oneshot_t newtask = {
			remaining_time, max_time, is_running, callback, priority, args,
		};
		system_tasks.push(newtask);
	}
	else {
		oneshot_t newtask = {
			remaining_time, max_time, is_running, callback, priority, args,
		};
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

	// update each task's remaining time, and identify the first ready task (if there is one).
	for (i = 0; i < MAXTASKS; i++){
		if (periodic_tasks[i].is_running){
			// update the task's remaining time
			periodic_tasks[i].remaining_time -= elapsed;
			if (periodic_tasks[i].remaining_time <= 0){
				if (t == NULL){
					// if this task is ready to run, and we haven't already selected a task to run,
					// select this one.
					t = periodic_tasks[i].callback;
					args = periodic_tasks[i].args;
					periodic_tasks[i].remaining_time += periodic_tasks[i].period;
				}
				idle_time = 0;
			}
			else {
				idle_time = min((uint32_t)periodic_tasks[i].remaining_time, idle_time);
			}
		}
	}
	if (t != NULL){
		// If a task was selected to run, call its function.
		t(args);
	}
	return idle_time;
}

uint32_t Scheduler_Dispatch_Oneshot(uint32_t idle_time){
	uint32_t now = current_tic;
	uint32_t elapsed = now - last_runtime;
	oneshot_t next_task;
	bool nexttask_allocated = false;

	if(!system_tasks.empty()){
		if(system_tasks.front()->is_running){
			system_tasks.front()->remaining_time -= elapsed;
			} else {
			if(system_tasks.front()->max_time < idle_time)
			next_task = *system_tasks.front();
			nexttask_allocated  = true;
		}
		} else if(!oneshot_tasks.empty()){
		if(oneshot_tasks.front()->is_running){
			oneshot_tasks.front()->remaining_time -= elapsed;
			} else {
			if(oneshot_tasks.front()->max_time < idle_time)
			next_task = *oneshot_tasks.front();
			nexttask_allocated = true;
		}
	}
	if(nexttask_allocated) Scheduler_RunTask_Oneshot(next_task);
}

uint32_t Scheduler_RunTask_Oneshot(oneshot_t next_task){
	// run the task
	next_task.is_running = 1;
	next_task.callback(next_task.args);
	//

	// --- task is running, could be interrupted?

	// task is done running:
	if(next_task.priority){
		system_tasks.pop();
		} else {
		oneshot_tasks.pop();
	}
}