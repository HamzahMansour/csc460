#include "scheduler.h"

#include "arduino/Arduino.h"
#include <avr/interrupt.h>

typedef struct // period tasks
{
	int32_t period;
	int32_t remaining_time;
	uint8_t is_running;
	task_cb callback;
	object args;
} task_t;

typedef struct { // one-shot tasks
	int32_t remaining_time;
	int32_t max_time;
	uint8_t is_running;
	task_cb callback;
	int priority;
	object args;
} oneshot_t;

task_t periodic_tasks[MAXTASKS];
std::queue <oneshot_t> oneshot_tasks;
std::queue <oneshot_t> system_tasks;

uint32_t last_runtime;

void Scheduler_Init()
{
	last_runtime = millis();
}

void Scheduler_StartPeriodicTask(int16_t delay, int16_t period, task_cb task, object args=NULL)
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

void Schedule_OneshotTask(int32_t remaining_time,	int32_t max_time,	uint8_t is_running,	task_cb callback,	int priority=0,	object args=NULL){
	if (priority)
	{
		oneshot_t newtask = {
			.remaining_time = remaining_time,
			.max_time = max_time,
			.is_running = is_running,
			.callback = callback,
			.priority = priority,
			.args = args,
		};
		system_tasks.push(newtask);
	}
	else {
		oneshot_t newtask = {
			.remaining_time = remaining_time,
			.max_time = max_time,
			.is_running = is_running,
			.callback = callback,
			.priority = priority,
			.args = args,
		};
		oneshot_tasks.push(newtask);
	}
}

uint32_t Scheduler_Dispatch_Periodic()
{
	uint8_t i;

	uint32_t now = millis();
	uint32_t elapsed = now - last_runtime;
	last_runtime = now;
	task_cb t = NULL;
	uint32_t idle_time = 0xFFFFFFFF;
	object args = NULL;

	// update each task's remaining time, and identify the first ready task (if there is one).
	for (i = 0; i < MAXTASKS; i++){
		if (periodic_tasks[i].is_running)
		{
			// update the task's remaining time
			periodic_tasks[i].remaining_time -= elapsed;
			if (periodic_tasks[i].remaining_time <= 0)
			{
				if (t == NULL)
				{
					// if this task is ready to run, and we haven't already selected a task to run,
					// select this one.
					t = periodic_tasks[i].callback;
					args = periodic_tasks[i].args;
					periodic_tasks[i].remaining_time += periodic_tasks[i].period;
				}
				idle_time = 0;
			}
			else
			{
				idle_time = min((uint32_t)periodic_tasks[i].remaining_time, idle_time);
			}
		}
	}
	if (t != NULL)
	{
		// If a task was selected to run, call its function.
		if(args != NULL){
			t(args);
		} else{
			t();
		}
	}
	return idle_time;
}

uint32_t Scheduler_Dispatch_Oneshot(){
	if(!system_tasks.empty()){
		if(system_tasks.front().max_time < idle_time){
			// run high priority task

		}
	} else if(!oneshot_tasks.empty()){
		if(oneshot_tasks.front().max_time < idle_time){
			// run low priority task

		}
	}
}
