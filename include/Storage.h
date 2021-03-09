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
	Storage(size_t _schema[], int _schemaSize, int _startAddress = 0);
	bool set(int index, void* val, int mode = 1, bool update = true); // Sets the value at the schema index. `mode`: 0 - don't commit, 1 - commit, 2 - end
	void get(int index, uint8_t buf[]); // Gets the value at the schema index
	bool clear(int index, bool commit = true); // Clears the value at the schema index (Sets all bytes to 255)
	void clearAll(bool commit = true); // Clears the entire address space
	void printMemory(); // Prints the EEPROM values to Serial
};

#endif