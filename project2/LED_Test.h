// LED_Test.h
#ifndef TEST_H_
#define TEST_H_

#ifdef __cplusplus
extern "C" {
	#endif
void initB();
void disableB();
void enableB(unsigned int mask);
void initE();
void disableE();
void enableE(unsigned int mask);
void taskAunispawn(LinkedList<arg_t> &obj);
void taskAmultispawn(LinkedList<arg_t> &obj);
void taskDunispawn(LinkedList<arg_t> &obj);
void taskDmultispawn(LinkedList<arg_t> &obj);
void taskB(LinkedList<arg_t> &obj);
void taskC(LinkedList<arg_t> &obj);
void test_taskClash();

void test_impossiblePeriod();

void test_deceptiveOneshot();

void test_tooLongSystemTask();

void test_tooLongOneshotTask();


#ifdef __cplusplus
	}
	#endif

#endif
