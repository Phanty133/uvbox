#include "Melody.h"

#define MELODYSIZE 5 // The max number of notes you can have in a melody because I am still lazy

Melody::Melody(){
	this->bpm = 60;
	this->buzzerPin = 0;
}

Melody::Melody(int _buzzerPin, TimerSet* _timers, int _bpm, float _notes[MELODYSIZE]){
	this->bpm = _bpm;
	this->eigthNote = 30000 / bpm;
	this->timers = _timers;
	this->buzzerPin = _buzzerPin;

	for(int i = 0; i < MELODYSIZE; i++){
		this->notes[i] = _notes[i];
	}
}

void Melody::play(){
	int i = 0;

	for(i = 0; i < MELODYSIZE; i++){
		if(this->notes[i] == -1) break; // If we encounter -1 (stop command), break

		// Allocate space for the frequency and the pin (There probably is a better way of doing it) (It is freed once the callback is called)
		void* arg = malloc(sizeof(float) + sizeof(uint8_t)); 
		*(float*)arg = this->notes[i]; // Set the float space to the frequency
		*((uint8_t*)(arg + sizeof(float))) = this->buzzerPin; // Set the integer space to the buzzer pin

		this->timers->addTimerWithArg(this->eigthNote * i, [](void* argPtr){	
			if(*(float*)argPtr == 0){
				noTone(*((uint8_t*)(argPtr + sizeof(float))));
			}
			else{
				tone(*((uint8_t*)(argPtr + sizeof(float))), *(float*)argPtr);
			}
		}, arg);
	}

	// After the last note turn off the buzzer
	this->timers->addTimerWithArg(this->eigthNote * i, [](void* pinPtr){
		noTone(*(uint8_t*)pinPtr);
	}, (void*)&(this->buzzerPin), false); 
}
