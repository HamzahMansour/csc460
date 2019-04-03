/*
 * AVRUtilities.h
 *
 * Created: 2019-04-01 11:32:45 AM
 *  Author: chris
 */ 


#ifndef AVRUTILITIES_H_
#define AVRUTILITIES_H_
#include <stdint.h>

void adc_init(void);
uint16_t read_adc(uint8_t channel);
int map(float value, float start1, float stop1, float start2, float stop2);
void PWM_Init_Pan();
void PWM_Init_Tilt();
void PWM_write_Pan(int us);
void PWM_write_Tilt(int us);
#endif /* AVRUTILITIES_H_ */