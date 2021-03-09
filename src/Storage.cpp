#include "Storage.h"

Storage::Storage(){}

Storage::Storage(size_t _schema[], int _schemaSize, int _startAddress){
	this->startAddress = _startAddress;
	this->schemaSize = _schemaSize;

	for(int i = 0; i < this->schemaSize; i++){
		this->schema[i] = _schema[i];
		this->size += this->schema[i];

		if(i == 0){
			this->addressMap[i] = _startAddress;
		}
		else{
			this->addressMap[i] = this->addressMap[i - 1] + this->schema[i - 1]; // Set the address to be right after the previous address
		}
	}
}

bool Storage::set(int index, void* valptr, int mode, bool update){ // Returns false if the index is out of range of the schema
	if(index >= this->schemaSize) return false;

	for(int i = 0; i < this->schema[index]; i++){
		uint8_t val = *((uint8_t*)(valptr + i));
		int addr = this->addressMap[index] + i;

		if(update){
			if(EEPROM.read(addr) != val) EEPROM.write(addr, val);
		}
		else{
			EEPROM.write(addr, val);
		}
	}

	switch(mode){
		case 1:
			EEPROM.commit();
			break;
		case 2:
			EEPROM.end();
			break;
	}

	return true;
}

void Storage::get(int index, uint8_t buf[]){ 
	for(int i = 0; i < this->schema[index]; i++){
		buf[i] = EEPROM.read(this->addressMap[index] + i);
	}
}

bool Storage::clear(int index, bool commit){ // Returns false if the index is out of range of the schema
	if(index >= this->schemaSize) return false;

	for(int i = 0; i < this->schema[index]; i++){
		EEPROM.write(this->addressMap[i] + i, 0);
	}

	if(commit) EEPROM.commit();
	return true;
}

void Storage::clearAll(bool commit){
	for(int i = 0; i < this->size; i++){
		EEPROM.write(this->startAddress + i, 0);
	}

	if(commit) EEPROM.commit();
}

void Storage::printMemory(){
	for(int i = 0; i < this->schemaSize; i++){
		Serial.print(i);
		Serial.print(": ");

		for(int addr = 0; addr < this->schema[i]; addr++){
			Serial.print(EEPROM.read(this->addressMap[i] + addr));
			Serial.print(" ");
		}

		Serial.println();
	}
}
