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

	#ifdef __cplusplus
}
#endif

#endif