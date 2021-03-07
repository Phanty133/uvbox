#include "Storage.h"

Storage::Storage(){}

Storage::Storage(size_t _schema[MAXSCHEMASIZE], int _startAddress){
	this->startAddress = _startAddress;
	this->schemaSize = sizeof(_schema) / sizeof(size_t) + 1;

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

bool Storage::set(int index, void* val, bool commit){ // Returns false if the index is out of range of the schema
	if(index >= this->schemaSize) return false;
	
	for(int i = 0; i < this->schema[index]; i++){
		EEPROM.write(this->addressMap[index] + i, *(uint8_t*)(val + i));	
	}

	if(commit) EEPROM.commit();
	return true;
}

void* Storage::get(int index){ 
	uint8_t values[this->schema[index]];

	for(int i = 0; i < this->schema[index]; i++){
		values[i] = EEPROM.read(this->addressMap[index] + i);
	}

	return (void*)&values;
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
