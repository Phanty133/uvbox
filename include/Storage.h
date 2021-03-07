#ifndef _STORAGE_H
#define _STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>

#define MAXSCHEMASIZE 20 // Laziness ensues

struct Storage{ // A simple EEPROM data manager because I don't want to remember the address in EEPROM for everything
	int startAddress; // Starting adress of the storage 
	int size = 0; // Size of the storage in bytes
	size_t schema[MAXSCHEMASIZE] = {0}; // Storage schema - Each element is the size of the value to be stored in EEPROM
	int addressMap[MAXSCHEMASIZE] = {0}; // Starting EEPROM addresses of values in schema
	int schemaSize = 0; // Just for convenience, so I don't have to write an expression each time

	Storage();
	Storage(size_t _schema[MAXSCHEMASIZE], int _startAddress = 0);
	bool set(int index, void* val, bool commit = true); // Sets the value at the schema index. Set `commit` to false to not automatically commit to EEPROM after values have been set
	void* get(int index); // Gets the value at the schema index
	bool clear(int index, bool commit = true); // Clears the value at the schema index (Sets all bytes to 255)
	void clearAll(bool commit = true); // Clears the entire address space
};

#endif