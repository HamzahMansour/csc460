#include <avr/io.h>
#include "LED_Test.h"

void initB(){
	DDRB = 0xFF;
	PORTB = 0x00;
}

void disableB(){
	PORTB = 0x00;
}

void enableB(unsigned int mask){
	PORTB = mask;
}
