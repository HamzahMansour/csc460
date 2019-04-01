/*
 * AVRUtilities.cpp
 *
 * Created: 2019-04-01 11:32:19 AM
 *  Author: chris
 */ 
#include "AVRUtilities.h"
#include "avr/io.h"

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
// int called = 0;
// 
// private void PWM_Init(){
// 	DDRD |= 0xFF;
// 	called = 1;
// }
// 
// public void PWM_Init_Lazer(){
// 	if(!called) PWM_Init();
// 	TCCR2A |= 1<<WGM21 | 1<<WGM20 | 1<<CS20;
// 	TCCR2B |= 1<<WGM21 | 1<<WGM20 | 1<<CS20;
// 	OCR2A = 0;
// 	OCR2B = 0;
// 	DDRD |= (1<<PD0)
// }
// 
// public void set_duty_Lazer(int duty){
// 	
// }