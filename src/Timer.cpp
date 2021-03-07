#include "Timer.h"

Timer::Timer(){
	init = false;
}

Timer::Timer(float _length, void (*_cb) ()){
	this->startTime = millis();
	this->length = _length;
	this->cb = _cb;
	this->endTime = startTime + length;
	this->cbHasArg = false;

	this->init = true;
}

Timer::Timer(float _length, void (*_cb)(void*), void* _cbArg, bool freeArg){
	this->startTime = millis();
	this->length = _length;
	this->cbWithArg = _cb;
	this->cbArg = _cbArg;
	this->endTime = startTime + length;
	this->cbHasArg = true;
	this->freeArg = freeArg;

	this->init = true;
}

bool Timer::check(){
	if(millis() >= this->endTime){
		if(this->cbHasArg){
			this->cbWithArg(this->cbArg);
			if(this->freeArg) free(this->cbArg);
		}
		else{
			this->cb();
		}

		return true;
	}
		
	return false;
}
