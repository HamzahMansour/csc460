// main.c UVic CSC460 Spring 2019 Team 13 
#define F_CPU 16000000UL	
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "uart/uart.h"
#include "lcd/lcd.h"

#define     clock8MHz()    cli(); CLKPR = _BV(CLKPCE); CLKPR = 0x00; sei();
volatile uint8_t rxflag = 0;

uint8_t joystick1X_PIN = 14;
uint8_t joystick1Y_PIN = 15;
uint8_t joystick2X_PIN = 12;
uint8_t joystick2Y_PIN = 13;

uint16_t joystick1X = 513;
uint16_t joystick1Y = 507;
uint8_t joystick1Z = 1;
uint16_t joystick2X = 493;
uint16_t joystick2Y = 506;
uint8_t joystick2Z = 1;

char printfStr[16];

int panSpeed = 0, maxAdjustPan = 40;
int tiltSpeed = 0, maxAdjustTilt = 10;
int velSpeed = 0, maxAdjustVel = 50;
int radSpeed = 0, maxAdjustRad = 50;

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

void joystickRead(){
	joystick1X = read_adc(joystick1X_PIN); // do each twice ?
	joystick1Y = read_adc(joystick1Y_PIN); 
	joystick1Z = PINA & (1<<PINA0);
	joystick2X = read_adc(joystick2X_PIN);
	joystick2Y = read_adc(joystick2Y_PIN);
	joystick2Z = PINA & (1<<PINA1);
}

void lcdPrint(){
	sprintf(printfStr,"1X%4d Y%4d Z%2d",joystick1X,joystick1Y,joystick1Z);
	lcd_puts(printfStr);
	sprintf(printfStr,"2X%4d Y%4d Z%2d",joystick2X,joystick2Y,joystick2Z);
	lcd_puts(printfStr);
}

void writeTwo(int a, int b){
//	Serial1.write(a);
//	Serial1.write(b);
}

void panHandle(int panInput){
	int oldSpeed = panSpeed;
	if(panInput > 540){
		panSpeed = map(panInput, 540, 1021, 0, -maxAdjustPan);
	}
	else if (panInput < 440){
		panSpeed = map(panInput, 440, 0, 0, maxAdjustPan);
	}
	else{
		panSpeed = 0;
	}

	if(panSpeed != oldSpeed){
		int byte = map(panSpeed, -maxAdjustPan, maxAdjustPan-1, 3, 84);
		char *sendString;
		sprintf(sendString, "1%d", byte);
// 		uart_putstr(sendString, CONTROL_UART_1);
	}
}

void tiltHandle(int tiltInput){
	int oldSpeed = tiltSpeed;

	if(tiltInput > 555){
		tiltSpeed = map(tiltInput, 555, 1022, 0, maxAdjustTilt);
	}
	else if(tiltInput < 425){
		tiltSpeed = map(tiltInput, 425, 0, 0, -maxAdjustTilt);
	}
	else{
		tiltSpeed = 0;
	}

	if(tiltSpeed != oldSpeed){
		int byte = map(tiltSpeed, -maxAdjustTilt, maxAdjustTilt-1, 1, 22);
		char *sendString;
		sprintf(sendString, "1%d", byte);
// 		uart_putstr(sendString, CONTROL_UART_1);
	}
}

void velocityHandle(int velInput){
	int oldVelocity = velSpeed;
	
	if(velInput > 520){
		velSpeed = map(velInput, 520, 1023, 0, maxAdjustVel);
	}
	else if(velInput < 480){
		velSpeed = map(velInput, 480, 0, 0, -maxAdjustVel);
	}
	else {
		velSpeed = 0;
	}
	
	if(velSpeed != oldVelocity){
		int byte = map(velSpeed, -maxAdjustVel, maxAdjustVel-1, 1, 100);
		char *sendString;
		sprintf(sendString, "1%d", byte);
// 		uart_putstr(sendString, CONTROL_UART_1);
	}
}

void radiusHandle(int radInput){
	int oldRadius = radSpeed;
	
	if(radInput > 520){
		radSpeed = map(radInput, 520, 1023, 0, maxAdjustRad);
	}
	else if(radInput < 480){
		radSpeed = map(radInput, 480, 0, 0, -maxAdjustRad);
	}
	else {
		radSpeed = 0;
	}
	
	if(radSpeed != oldRadius){
		int byte = map(radSpeed, -maxAdjustRad, maxAdjustRad-1, 1, 100);
		char *sendString;
		sprintf(sendString, "1%d", byte);
// 		uart_putstr(sendString, CONTROL_UART_1);
	}
}

int main()
{
	clock8MHz();
	cli();
	DDRB = 0xFF;
	DDRK = 0x00; // set port K as input
	DDRG = 0x00; // set port G as input
	DDRA = 0x00; // set port A as input (using pins 4 and 6)
	PORTA = 0xFF; // set input_pullup for all port A pins
	adc_init();
	lcd_init();
	uart_init(UART_9600);
	sei();
	
	for(;;){
		joystickRead();
		//lcdPrint();	
		
		char *joystickSend;
		if(joystick1Z){
			joystickSend = "11";
			/*uart_putstr(joystickSend, CONTROL_UART_1);*/
			uart_putchar('1');
/*			uart_putchar('1', CONTROL_UART_2);*/
			lcd_putchar('0');
		}
		else{
			joystickSend = "12";
			/*uart_putstr(joystickSend, CONTROL_UART_1);*/
			uart_putchar('2');
/*			uart_putchar('2', CONTROL_UART_2);*/
			lcd_putchar('1');
		}

		char byte1 = uart_get_byte(0);
		char byte2 = uart_get_byte(1);
		
		if (byte1 == 1) PORTB = 0b10000000;
		else PORTB = 0x00;
		
// 		lcd_putchar(getbyte1);
// 		lcd_blank(15);
// 		lcd_putchar(getbyte2);
// 		lcd_blank(15);
		_delay_ms(50);
	}
	return 0;
}