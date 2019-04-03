/*
 * AVRUtilities.cpp
 *
 * Created: 2019-04-01 11:32:19 AM
 *  Author: chris
 */ 
#include "AVRUtilities.h"
#include "avr/io.h"

#define cli()		asm volatile ("cli"::)
#define sei()		asm volatile ("sei"::)

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
int map(float value, float start1, float stop1, float start2, float stop2) {
	 int outgoing = start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
	 return outgoing;
 }


void PWM_Init_Pan(){
	cli();
	DDRE |= (1<< PE5);
	
	TCCR3A = 0;
	TCCR3B = 0;
	TIMSK3 &= ~(1 << OCIE3C);
	
	// fast pwm mode
	TCCR3A |= 1<<WGM31 | 1<<WGM30 | 1<<COM3C1;
	TCCR3B |= 1<<WGM32 | 1<<WGM33 | 1<<CS01;
	
	OCR3A = 40000; // 20000 us period
	OCR3B = 3000; // target high
	sei();
}

void PWM_Init_Tilt(){
	cli();
	DDRH |= (1<< PH5);
	
	TCCR4A = 0;
	TCCR4B = 0;
	TIMSK4 &= ~(1 << OCIE4C);
	
	// fast pwm mode
	TCCR4A |= 1<<WGM41 | 1<<WGM40 | 1<<COM4C1;
	TCCR4B |= 1<<WGM42 | 1<<WGM43 | 1<<CS01;
	
	OCR4A = 40000; // 20000 us period
	OCR4B = 3000; // target high
	sei();
}

void PWM_write_Pan(int us){
	if(us < 500) us = 500;
	else if(us > 2500) us = 2500;
	cli();
	OCR3C = us*2;
	sei();
}

void PWM_write_Tilt(int us){
	if(us < 500) us = 500;
	else if(us > 2500) us = 2500;
	cli();
	OCR4C = us*2;
	sei();
}