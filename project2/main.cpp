#include "arduino/Arduino.h"
#include "scheduler.h"

uint8_t pulse1_pin = 3;
uint8_t pulse2_pin = 4;
uint8_t idle_pin = 7;

// task function for PulsePin task
void pulse_pin1_task()
{
	digitalWrite(pulse1_pin, HIGH);

	digitalWrite(pulse1_pin, LOW);
}

// task function for PulsePin task
void pulse_pin2_task()
{
	digitalWrite(pulse2_pin, HIGH);

	digitalWrite(pulse2_pin, LOW);
}

// idle task
void idle(uint32_t idle_period)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.  For example, it
	// could sleep or respond to I/O.

	// example idle function that just pulses a pin.
	digitalWrite(idle_pin, HIGH);
	delay(idle_period);
	digitalWrite(idle_pin, LOW);
}

void setup()
{
	pinMode(pulse1_pin, OUTPUT);
	pinMode(pulse2_pin, OUTPUT);
	pinMode(idle_pin, OUTPUT);

	Scheduler_Init();

	// Start task arguments are:
	//		start offset in ms, period in ms, function callback

	Scheduler_StartTask(0, 500, pulse_pin1_task);
	Scheduler_StartTask(0, 300, pulse_pin2_task);
}

void loop()
{
	uint32_t idle_period = Scheduler_Dispatch();
	if (idle_period)
	{
		idle(idle_period);
	}
}

int main()
{
	init();
	setup();

	for (;;)
	{
		loop();
	}
	for (;;);
	return 0;
}
