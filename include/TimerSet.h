#ifndef _TIMERSET_H
#define _TIMERSET_H

#include "Timer.h"
#include <Arduino.h>

#define TIMERSETSIZE 25 // The max number of timers you can have at one time because I am lazy

struct TimerSet { // A simple container for timers
	Timer* timers[TIMERSETSIZE];

	TimerSet();
	int firstFreeIndex();
	int addTimer(float length, void (*cb)());
	int addTimerWithArg(float length, void (*cb)(void*), void* cbArg, bool freeArg = true);
	void stopTimer(int i);
	void check();
};

#endif
