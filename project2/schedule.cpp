#include "scheduler.h"
#include <avr/interrupt.h>

typedef struct {
	char port;
	uint8_t pin;
} arg_t;

typedef struct // period tasks
{
	int32_t period;
	int32_t remaining_time;
	uint8_t is_running;
	task_cb callback;
	arg_t args[4];
} task_t;

typedef struct { // one-shot tasks
	int32_t remaining_time;
	int32_t max_time;
	uint8_t is_running;
	task_cb callback;
	int priority;
	arg_t args[4];
} oneshot_t;

task_t periodic_tasks[MAXTASKS];
queue <oneshot_t> oneshot_tasks;
queue <oneshot_t> system_tasks;

uint32_t last_runtime;

void Scheduler_Init()
{
	// last_runtime = millis(); // timer instead
}

void Scheduler_StartPeriodicTask(int16_t delay, int16_t period, task_cb task, arg_t args[4]=NULL)
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

void Schedule_OneshotTask(int32_t remaining_time,	int32_t max_time,	uint8_t is_running,	task_cb callback,	int priority=0,	arg_t args[4]=NULL){
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
		system_tasks.push_back(newtask);
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
		oneshot_tasks.push_back(newtask);
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
	arg_t args[4] = NULL;

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
		if(args != NULL){
			t(args);
		} else{
			t();
		}
	}
	return idle_time;
}

uint32_t Scheduler_Dispatch_Oneshot(uint32_t idle_time){
	task_cb t = NULL;
	uint32_t now = millis();
	uint32_t elapsed = now - last_runtime;
	oneshot_t next_task = NULL;

	if(!system_tasks.empty()){
		if(system_tasks.front().is_running){
			system_tasks.front().remaining_time -= elapsed;
		} else {
			if(system_tasks.front().max_time < idle_time)
					next_task = system_tasks.front();
		}
	} else if(!oneshot_tasks.empty()){
			if(oneshot_tasks.front().is_running){
				oneshot_tasks.front().remaining_time -= elapsed;
			} else {
				if(oneshot_tasks.front().max_time < idle_time)
						next_task = oneshot_tasks.front();
			}
	}
	if(next_task) Scheduler_RunTask_Oneshot(next_task);
}

uint32_t Scheduler_RunTask_Oneshot(oneshot_t next_task){
	// run the task
	next_task.is_running = 1;
	if(next_task.args != NULL){
		next_task.callback(next_task.args);
	} else{
		next_task.callback();
	}
//

// --- task is running, could be interrupted?

// task is done running:
	if(next_task.priority){
		system_tasks.pop_front();
	} else {
		oneshot_tasks.pop_front();
	}
}
