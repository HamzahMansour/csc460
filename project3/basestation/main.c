// main.c UVic CSC460 Spring 2019 Team 13 
#define F_CPU 16000000UL	
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "uart/uart.h"
#include "lcd/lcd.h"
/*#include "schedule/scheduler.h"*/

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();
volatile uint8_t rxflag = 0;

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
uint16_t joystick1X = 513;
uint16_t joystick1Y = 507;
uint8_t joystick1Z = 1;
uint16_t joystick2X = 493;
uint16_t joystick2Y = 506;
uint8_t joystick2Z = 1;

// LCD Shield values
int buttonPress = 0;
int printType = INPUT;
char printfStr[16];

// 2B bluetooth communication protocol
char sendType = 0;
char sendByte = 0;
	// sendType, sendByte:
	// 0: laser - 0: off, 1: on
	// 1: pan - 3:84
	// 2: tilt - 1:22
	// 3: velocity - 1:100
	// 4: radius - 1:100

// output values
int panSpeed = 0, maxAdjustPan = 40;
int tiltSpeed = 0, maxAdjustTilt = 10;
int velSpeed = 0, maxAdjustVel = 50;
int radSpeed = 0, maxAdjustRad = 50;

// from bluetooth
int state;
int timeLeft;
int shot;

 /*source: https://bennthomsen.wordpress.com/arduino/peripherals/analogue-input/ */
void adc_init(void){
	//16MHz/128 = 125kHz the ADC reference clock
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));
	ADMUX |= (1<<REFS0);       //Set Voltage reference to Avcc (5v)
	ADCSRA |= (1<<ADEN);       //Turn on ADC
	ADCSRA |= (1<<ADSC);      //Do an initial conversion
}

uint16_t read_adc(uint8_t channel){
	ADMUX &= 0xE0;           //Clear bits MUX0-4
	ADMUX |= channel&0x07;   //Defines the new ADC channel to be read by setting bits MUX0-2
	ADCSRB = channel&(1<<3); //Set MUX5
	ADCSRA |= (1<<ADSC);      //Starts a new conversion
	while(ADCSRA & (1<<ADSC));  //Wait until the conversion is done
	return ADCW;         //Returns the ADC value of the chosen channel
}

 /*source: https://github.com/processing/processing/blob/be7e25187b289f9bfa622113c400e26dd76dc89b/core/src/processing/core/PApplet.java#L5061 */
 static int map(float value, float start1, float stop1, float start2, float stop2) {
	  int outgoing = start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
	  return outgoing;
 }

void joystickRead(){
	joystick1X = read_adc(joystick1X_PIN); // do each twice ?
	joystick1Y = read_adc(joystick1Y_PIN); 
	joystick1Z = 1>>(PINA & (1<<PINA0));
	joystick2X = read_adc(joystick2X_PIN);
	joystick2Y = read_adc(joystick2Y_PIN);
	joystick2Z = 1>>(PINA & (1<<PINA1));
}

void lcdPrint(){
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
			sprintf(printfStr,"ROOMBA STATE/ IR");
			lcd_puts(printfStr);
			sprintf(printfStr,"ENERGY LEFT     ");
			lcd_puts(printfStr);
			break;			
		default:
			lcd_blank(32);
			break;
	}	
}

 /*source: https://www.dfrobot.com/wiki/index.php/LCD_KeyPad_Shield_For_Arduino_SKU:_DFR0009 */
void lcdButtons(){
	int buttonReading = read_adc(0); 
	if (buttonReading < 50) buttonPress = 1; //RIGHT
	else if (buttonReading < 250) buttonPress = 2; //UP
	else if (buttonReading < 450) buttonPress = 3; //DOWN
	else if (buttonReading < 650) buttonPress = 4; //LEFT
	else if (buttonReading < 850) buttonPress = 5; //SELECT
	else buttonPress = 0;
}

// void writeTwo(char a, char b){
// 	uart_putchar(a, CH_2);
// 	uart_putchar(b, CH_2);
// }

void sendBluetooth(){
	uart_putchar(sendType, CH_2);
	uart_putchar(sendByte, CH_2);
}

void laserHandle(){
	sendType = 0;
	sendByte = joystick1Z;
// 	if(joystick1Z == 0) sendByte = 0; //laser off
// 	else if(joystick1Z > 0) sendByte = 1; //laser on
	sendBluetooth(); // make into a separate one-shot task?
}

void panHandle(){
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

void tiltHandle(){
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

void velocityHandle(){
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

void radiusHandle(){
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

int main()
{
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
	
	for(;;){
		// these will be periodic tasks
		joystickRead();
		lcdButtons();
		lcdPrint();
		laserHandle();
		panHandle();
		tiltHandle();
		velocityHandle();
		radiusHandle();
		
		_delay_ms(50);
	}
	return 0;
}