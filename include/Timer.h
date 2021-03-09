#ifndef _TIMER_H
#define _TIMER_H

#include <Arduino.h>
#define TIMERSETSIZE 25

struct Timer { // A simple timer
	float length;
	void (*cb)();
	float startTime;
	float endTime;

	void (*cbWithArg)(void*);
	void* cbArg;
	bool cbHasArg;
	bool freeArg;

	bool init = false;

	Timer();
	Timer(float _length, void (*_cb) ());
	Timer(float _length, void (*_cb)(void*), void* _cbArg, bool freeArg = true); // If the callback needs an argument
	bool check();
	int timeRemaining();
};

#endif
