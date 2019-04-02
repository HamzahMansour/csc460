// main.cpp UVic CSC460 Spring 2019 Team 13
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stddef.h>

#include "string.h"
#include "uart/uart.h"
#include "lcd/lcd.h"
#include "TTA/scheduler.h"
#include "util/AVRUtilities.h"

LinkedList<arg_t> obj;
LinkedList<arg_t> obj1;

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();

typedef enum print_type
{
	INPUT,
	OUTPUT,
	COMPETE,
} PRINT_TYPE;

// keep as global variables?

// input pin values
uint8_t joystick1X_PIN = 14;
uint8_t joystick1Y_PIN = 15;
uint8_t joystick2X_PIN = 12;
uint8_t joystick2Y_PIN = 13;

// input values
int joystick1X = 513;
int joystick1Y = 507;
int joystick1Z = 1;
int joystick2X = 493;
int joystick2Y = 506;
int joystick2Z = 1;

// LCD Shield values
int buttonPress = 0;
int printType = COMPETE;
char printfStr[16];

// 2B bluetooth communication protocol
char sendType = 0;
char sendByte = 0;
// sendType, sendByte:
// 	0: laser - 0: off, 1: on
// 	1: pan - 3:84
// 	2: tilt - 1:22
// 	3: velocity - 1:100
// 	4: radius - 1:100

// output values
int panSpeed = 0, maxAdjustPan = 40;
int tiltSpeed = 0, maxAdjustTilt = 10;
int velSpeed = 0, maxAdjustVel = 50;
int radSpeed = 0, maxAdjustRad = 50;

// from bluetooth
int state = 1;
int timeLeft = 30;
int shot = 0;
int energy = 100;

void joystickRead(LinkedList<arg_t> &obj){
	joystick1X = read_adc(joystick1X_PIN); // do each twice ?
	joystick1Y = read_adc(joystick1Y_PIN);
	joystick1Z = 1>>(PINA & (1<<PINA0));
	joystick2X = read_adc(joystick2X_PIN);
	joystick2Y = read_adc(joystick2Y_PIN);
	joystick2Z = 1>>(PINA & (1<<PINA1));
}

void lcdPrint(LinkedList<arg_t> &obj){
	if(buttonPress == 5){ // if SELECT has been pressed
		if(printType == INPUT) printType = OUTPUT;
		else if(printType == OUTPUT) printType = COMPETE;
		else printType = INPUT;
		buttonPress = 0; // reset button state
	}
	
	lcd_blank(32); // clear lcd first
	
	switch(printType){
		case INPUT:
			sprintf(printfStr,"JOY1X%4dY%4dZ%1d",joystick1X,joystick1Y,joystick1Z);
			lcd_puts(printfStr);
			sprintf(printfStr,"JOY2X%4dY%4dZ%1d",joystick2X,joystick2Y,joystick2Z);
			lcd_puts(printfStr);
			break;
		case OUTPUT:
			sprintf(printfStr,"PAN%3dTILT%3d B%1d",panSpeed,tiltSpeed,joystick1Z);
			lcd_puts(printfStr);
			sprintf(printfStr,"RAD%3dVELO%3d B%1d",radSpeed,velSpeed,joystick2Z);
			lcd_puts(printfStr);
			break;
		case COMPETE:
			sprintf(printfStr,"STATE %1d  TIME %2d",state,timeLeft);
			lcd_puts(printfStr);
			sprintf(printfStr,"LS %1d  ENERGY %3d",shot,energy);
			lcd_puts(printfStr);
			break;
		default:
			lcd_blank(32);
			break;
	}
}

/* source: https://www.dfrobot.com/wiki/index.php/LCD_KeyPad_Shield_For_Arduino_SKU:_DFR0009 */
void lcdButtons(LinkedList<arg_t> &obj){
	int buttonReading = read_adc(0);
	if (buttonReading < 50) buttonPress = 1; //RIGHT
	else if (buttonReading < 250) buttonPress = 2; //UP
	else if (buttonReading < 450) buttonPress = 3; //DOWN
	else if (buttonReading < 650) buttonPress = 4; //LEFT
	else if (buttonReading < 850) buttonPress = 5; //SELECT
	else buttonPress = 0;
}

void sendBluetooth(){
	uart_putchar(sendType, CH_2);
	uart_putchar(sendByte, CH_2);
}

void readBluetooth(LinkedList<arg_t> &obj){
	if(uart_bytes_received(CH_2) < 2) return;	
}

void laserHandle(LinkedList<arg_t> &obj){
	sendType = 0;
	sendByte = joystick1Z;
	// 	if(joystick1Z == 0) sendByte = 0; //laser off
	// 	else if(joystick1Z > 0) sendByte = 1; //laser on
	sendBluetooth(); // make into a separate one-shot task?
}

void panHandle(LinkedList<arg_t> &obj){
	int oldSpeed = panSpeed;
	if(joystick1Y > 540){
		panSpeed = map(joystick1Y, 1021, 540, -maxAdjustPan, 0);
	}
	else if (joystick1Y < 440){
		panSpeed = map(joystick1Y, 440, 0, 0, maxAdjustPan);
	}
	else{
		panSpeed = 0;
	}

	if(panSpeed != oldSpeed){
		sendType = 1;
		sendByte = map(panSpeed, -maxAdjustPan, maxAdjustPan-1, 3, 84);
		sendBluetooth();
	}
}

void tiltHandle(LinkedList<arg_t> &obj){
	int oldSpeed = tiltSpeed;

	if(joystick1X > 555){
		tiltSpeed = map(joystick1X, 555, 1022, 0, -maxAdjustTilt);
	}
	else if(joystick1X < 425){
		tiltSpeed = map(joystick1X, 425, 0, 0, maxAdjustTilt);
	}
	else{
		tiltSpeed = 0;
	}

	if(tiltSpeed != oldSpeed){
		sendType = 2;
		sendByte = map(tiltSpeed, -maxAdjustTilt, maxAdjustTilt-1, 1, 22);
		sendBluetooth();
	}
}

void velocityHandle(LinkedList<arg_t> &obj){
	int oldVelocity = velSpeed;
	
	if(joystick2X > 520){
		velSpeed = map(joystick2X, 520, 1023, 0, -maxAdjustVel);
	}
	else if(joystick2X < 480){
		velSpeed = map(joystick2X, 480, 0, 0, maxAdjustVel);
	}
	else {
		velSpeed = 0;
	}
	
	if(velSpeed != oldVelocity){
		sendType = 3;
		sendByte = map(velSpeed, -maxAdjustVel, maxAdjustVel-1, 1, 100);
		sendBluetooth();
	}
}

void radiusHandle(LinkedList<arg_t> &obj){
	int oldRadius = radSpeed;
	
	if(joystick2Y > 520){
		radSpeed = map(joystick2Y, 520, 1023, 0, -maxAdjustRad);
	}
	else if(joystick2Y < 480){
		radSpeed = map(joystick2Y, 480, 0, 0, maxAdjustRad);
	}
	else {
		radSpeed = 0;
	}
	
	if(radSpeed != oldRadius){
		sendType = 4;
		sendByte = map(radSpeed, -maxAdjustRad, maxAdjustRad-1, 1, 100);
		sendBluetooth();
	}
}

void idle(uint32_t idle_time)
{
	Scheduler_Dispatch_Oneshot();
}

void loop()
{
	uint32_t idle_time = Scheduler_Dispatch_Periodic();
	if (idle_time)
	{
		idle(idle_time);
	}
}

void setup(){
	clock8MHz();
	cli();
	DDRK = 0x00; // set port K as input (analog - using pins 4,5,6,7)
	DDRF = 0x00; // set port F as input (analog - using pin 0)
	DDRA = 0x00; // set port A as input (digital - using pins 4 and 6)
	PORTA = 0xFF; // enable pull-up resistance for all port A pins (needed for joystick buttons)
	adc_init();
	lcd_init();
	uart_init(UART_9600, CH_2);
	Scheduler_Init();
	
	Scheduler_StartPeriodicTask(0, 50, joystickRead, obj);
	Scheduler_StartPeriodicTask(0, 50, lcdButtons, obj);
	Scheduler_StartPeriodicTask(0, 100, lcdPrint, obj);
	Scheduler_StartPeriodicTask(0, 50, laserHandle, obj);
	Scheduler_StartPeriodicTask(0, 50, panHandle, obj);
	Scheduler_StartPeriodicTask(0, 50, tiltHandle, obj);
	Scheduler_StartPeriodicTask(0, 50, velocityHandle, obj);
	Scheduler_StartPeriodicTask(0, 50, radiusHandle, obj);
	Scheduler_StartPeriodicTask(0, 50, readBluetooth, obj);
	
	sei();
}

int main(){	
	setup();
	for(;;){
		loop();		
	}
	for(;;);
	return 0;
}