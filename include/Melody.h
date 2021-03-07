#ifndef _MELODY_H
#define _MELODY_H

#include "TimerSet.h"
#define MELODYSIZE 5

struct Melody { 
	int bpm;
	float notes[MELODYSIZE] = {-1};
	int eigthNote;
	TimerSet* timers;
	int buzzerPin;

	Melody();
	Melody(int _buzzerPin, TimerSet* _timers, int _bpm, float _notes[MELODYSIZE]);
	void play();
};

#endif