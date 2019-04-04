// main.cpp UVic CSC460 Spring 2019 Team 13
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
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
	DEATH,
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
int joystick1Z = 0;
int joystick2X = 493;
int joystick2Y = 506;
int joystick2Z = 0;

// new input values
int new_joystick1X = 513;
int new_joystick1Y = 507;
int new_joystick1Z = 0;
int new_joystick2X = 493;
int new_joystick2Y = 506;
int new_joystick2Z = 0;

// LCD Shield values
int buttonPress = 0;
int screenChange = 0;
int printType = COMPETE;
char printfStr[16];

// 2B bluetooth communication protocol
char sendType = 0;
char sendByte = 0;
// sendType, sendByte:
// 	0: laser - 0: off, 1: on
// 	1: pan - 0:81
// 	2: tilt - 0:21
// 	3: velocity - 0:100
// 	4: radius - 0:100

// output values
int panSpeed = 0, maxAdjustPan = 40;
int tiltSpeed = 0, maxAdjustTilt = 10;
int velSpeed = 0, maxAdjustVel = 50;
int radSpeed = 0, maxAdjustRad = 50;

// for testing only
int velSpeedSend = 0;
int radSpeedSend = 0;

// from bluetooth
int state = 1;
int timeLeft = 30;
int shot = 0;
int energy = 100;
int death = 0;

void joystickRead(LinkedList<arg_t> &obj){
	new_joystick1X = read_adc(joystick1X_PIN); // do each twice ?
	new_joystick1Y = read_adc(joystick1Y_PIN);
	new_joystick1Z = 1>>(PINA & (1<<PINA0));
	new_joystick2X = read_adc(joystick2X_PIN);
	new_joystick2Y = read_adc(joystick2Y_PIN);
	new_joystick2Z = 1>>(PINA & (1<<PINA1));
	
	if(buttonPress == 5) screenChange = 1; // SELECT pressed
}

void lcdPrint(LinkedList<arg_t> &obj){
	if(screenChange && !buttonPress){ // if SELECT has been pressed then released
		if(printType == INPUT) printType = OUTPUT;
		else if(printType == OUTPUT) printType = COMPETE;
		else printType = INPUT;
		screenChange = 0;
	}
	if(death) printType = DEATH;
		
	switch(printType){
		case INPUT:
			sprintf(printfStr,"1 X%04d Y%04d Z%1d",joystick1X,joystick1Y,joystick1Z);
			lcd_puts(printfStr);
			sprintf(printfStr,"2 X%04d Y%04d Z%1d",joystick2X,joystick2Y,joystick2Z);
			lcd_puts(printfStr);
			break;
		case OUTPUT:
// 			sprintf(printfStr,"PAN%3dTILT%3d B%1d",panSpeed,tiltSpeed,joystick1Z);
// 			lcd_puts(printfStr);
// 			sprintf(printfStr,"RAD%3dVELO%3d B%1d",radSpeed,velSpeed,joystick2Z);
// 			lcd_puts(printfStr);
// 			break;
			sprintf(printfStr,"RAD%3dVELO%3d SB",radSpeedSend,velSpeedSend);
			lcd_puts(printfStr);
			sprintf(printfStr,"RAD%3dVELO%3d B%1d",radSpeed,velSpeed,joystick2Z);
			lcd_puts(printfStr);
			break;
		case COMPETE:
			sprintf(printfStr,"STATE %1d  TIME %2d",state,timeLeft);
			lcd_puts(printfStr);
			if(joystick1Z) sprintf(printfStr,"LS %1d  ENERGY=%03d",shot,energy);
			else sprintf(printfStr,"LS %1d  ENERGY %03d",shot,energy);
			lcd_puts(printfStr);
			break;
		case DEATH:
			sprintf(printfStr,"OOPS YOU'RE DEAD");
			lcd_puts(printfStr);
			sprintf(printfStr,"    SORRY :(    ");
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
	if(buttonReading > 1000) buttonPress = 0; //NONE
	else if (buttonReading < 50) buttonPress = 1; //RIGHT
	else if (buttonReading < 250) buttonPress = 2; //UP
	else if (buttonReading < 450) buttonPress = 3; //DOWN
	else if (buttonReading < 650) buttonPress = 4; //LEFT
	else if (buttonReading < 850) buttonPress = 5; //SELECT
}

void parseBytes(int byte1, int byte2){
	switch(byte1){
		case(0): // state
			state = byte2; // 0: stand-still | 1: cruise
			break;
		case(1): // timeLeft
			timeLeft = byte2+1; // 0-29 seconds
			break;
		case(2): // energy
			energy = byte2; // 0-100%
			break;
		case(3): // light sensor
			shot = byte2; // 0: safe | 1: shot
			break;
		case(4):
			death = byte2; // 0: life | 1: death
			break;
		default:
			break;
	}
}

void sendBluetooth(){
	uart_putchar(sendType, CH_2);
	uart_putchar(sendByte, CH_2);
}

void readBluetooth(LinkedList<arg_t> &obj){
	int count = uart_bytes_received(CH_2);
	if(count < 2) return;
	if(count % 2){
		for(int i=0; i<count-1; i+=2) parseBytes(uart_get_byte(i,CH_2),uart_get_byte(i+1,CH_2));
		uart_set_front(count, CH_2);
	}
	else for(int i=0; i<count; i+=2) parseBytes(uart_get_byte(i,CH_2),uart_get_byte(i+1,CH_2));		
}

void laserHandle(LinkedList<arg_t> &obj){
	if(new_joystick2Z == joystick2Z) return;
	
	joystick2Z = new_joystick2Z;
	sendType = 0;
	sendByte = joystick2Z;
	sendBluetooth();
	
}

void panHandle(LinkedList<arg_t> &obj){
	if(abs(new_joystick1Y - joystick1Y) < 20) return;
	joystick1Y = new_joystick1Y;
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
		sendByte = map(panSpeed, -maxAdjustPan, maxAdjustPan-1, 0, 81);
		sendBluetooth();
	}
}

void tiltHandle(LinkedList<arg_t> &obj){
	if(abs(new_joystick1X - joystick1X) < 20) return;
	joystick1X = new_joystick1X;
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
		sendByte = map(tiltSpeed, -maxAdjustTilt, maxAdjustTilt-1, 0, 21);
		sendBluetooth();
	}
}

void velocityHandle(LinkedList<arg_t> &obj){
	if(abs(new_joystick2X - joystick2X) < 20) return;
	joystick2X = new_joystick2X;
	int oldVelocity = velSpeed;
	
	if(joystick2X > 520){
		velSpeed = map(joystick2X, 520, 800, 0, -maxAdjustVel+35);
		velSpeed = map(joystick2X, 801, 1023, -maxAdjustVel+35, -maxAdjustVel);

	}
	else if(joystick2X < 480){
		velSpeed = map(joystick2X, 450, 200, 0, maxAdjustVel-35);
		velSpeed = map(joystick2X, 199, 0, maxAdjustVel-34,maxAdjustVel);
	}
	else {
		velSpeed = 0;
	}
	
	if(velSpeed != oldVelocity){
		sendType = 3;
		sendByte = map(velSpeed, -maxAdjustVel, maxAdjustVel, 0, 100);
		// 
		velSpeedSend = sendByte;
		sendBluetooth();
	}
}

void radiusHandle(LinkedList<arg_t> &obj){
	if(abs(new_joystick2Y - joystick2Y) < 20) return;
	joystick2Y = new_joystick2Y;	
	int oldRadius = radSpeed;
	
	if(joystick2Y > 520){
		radSpeed = map(joystick2Y, 520, 800, 0, -maxAdjustRad+35);
		radSpeed = map(joystick2Y, 801, 1023, -maxAdjustRad+35, -maxAdjustRad);
	}
	else if(joystick2Y < 480){
		radSpeed = map(joystick2Y, 480, 200, 0, maxAdjustRad-35);
		radSpeed = map(joystick2Y, 199, 0, maxAdjustRad-35, maxAdjustRad);
	}
	else {
		radSpeed = 0;
	}
	
	if(radSpeed != oldRadius){
		sendType = 4;
		sendByte = map(radSpeed, -maxAdjustRad, maxAdjustRad, 0, 100);
		radSpeedSend = sendByte;
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

	sei();
	
	Scheduler_Init();	
	Scheduler_StartPeriodicTask(0, 40,	joystickRead, obj);
	Scheduler_StartPeriodicTask(0, 50,	lcdButtons, obj);
	Scheduler_StartPeriodicTask(0, 100, lcdPrint, obj);
	Scheduler_StartPeriodicTask(0, 40,	laserHandle, obj);
	Scheduler_StartPeriodicTask(0, 50,	panHandle, obj);
	Scheduler_StartPeriodicTask(0, 50,	tiltHandle, obj);
	Scheduler_StartPeriodicTask(0, 100, velocityHandle, obj);
	Scheduler_StartPeriodicTask(0, 50,	radiusHandle, obj);
	Scheduler_StartPeriodicTask(0, 50,	readBluetooth, obj);
}

int main(){	
	setup();
	for(;;){
		loop();		
	}
	for(;;);
	return 0;
}