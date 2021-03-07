#include "TimerSet.h"

TimerSet::TimerSet(){}

int TimerSet::firstFreeIndex(){
	int i = 0;

	while(this->timers[i++] != nullptr || i >= TIMERSETSIZE) // Find the first free index
		;

	if(i >= TIMERSETSIZE) return -1;

	return i - 1;
}

int TimerSet::addTimer(float length, void (*cb)()){
	int i = this->firstFreeIndex();

	// TODO: implement a case if the array is full (for now let's just hope it doens't come to that)
	this->timers[i] = new Timer(length, cb); // Set the timer to the first free index

	return i;
}

int TimerSet::addTimerWithArg(float length, void (*cb)(void*), void* cbArg, bool freeArg){
	int i = this->firstFreeIndex();

	this->timers[i] = new Timer(length, cb, cbArg, freeArg); // Set the timer to the first free index

	return i;
}

void TimerSet::stopTimer(int i){
	delete this->timers[i];
	this->timers[i] = nullptr;
}

void TimerSet::check(){
	for(int i = 0; i < TIMERSETSIZE; i++){
		if(this->timers[i] == nullptr) continue;

		if(this->timers[i]->check()){ // If the timer expired, remove it from the array
			this->stopTimer(i);
		}
	}
}
