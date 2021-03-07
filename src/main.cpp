#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Declarations
#include "TimerSet.h"
#include "Melody.h" // Buzzer melodies are important. I swear.
#include "Storage.h"

#define LED 14
#define BUZZER 12
#define BUZZER_TIMES 5
#define BUZZER_LENGTH 500
#define BUZZER_FREQ 440
#define EEPROM_SIZE 128
#define SERIAL_BAUD 115200

enum Action{
	IDLE,
	CONNECT,
	CONNECTING,
	CONNECTED
} action;

char defaultSSID[32] = "UVBox";
char defaultPassword[32] = "securepassword";

TimerSet timers;
int lampTimerIndex;
bool connectToNetwork;

AsyncWebServer server(80);

// EEPROM storage manager

// -------- SCHEMA --------
// 0: bool connectToNetwork
// 1: bool staticIP
// 2: char[32] targetSSID
// 3: char[32] APPassword
// 4: IPAddress localIP (Set if staticIP == 1) 
// 5: IPAddress GatewayIP (Set if staticIP == 1)
// 6: IPAddress subnet (Set if staticIP == 1)
// ------------------------

size_t storageSchema[] = { sizeof(uint8_t), sizeof(uint8_t), sizeof(char) * 32, sizeof(char) * 32, sizeof(IPAddress), sizeof(IPAddress), sizeof(IPAddress) }; 
Storage storage(storageSchema);

// Some stuff for the buzzer

const int bpm = 180;

void buzzerOn(){
	tone(BUZZER, BUZZER_FREQ);
}

void buzzerOff(){
	noTone(BUZZER);
} 

Melody melodyStart;
Melody melodyActivation;
Melody melodyForceStop;
Melody melodyNetworkHost;
Melody melodyNetworkConnect;
Melody melodyNetworkInitializing;
Melody melodyInitError;

void buzzerAlarm(){
	for(int i = 0; i < BUZZER_TIMES * 2; i += 2){ 
		timers.addTimer(BUZZER_LENGTH * i, buzzerOn);
		timers.addTimer(BUZZER_LENGTH * (i + 1), buzzerOff);
	}
} 

// Lamp stuff

void lampOn(){
	digitalWrite(LED, HIGH);
}

void lampOff(){
	digitalWrite(LED, LOW);
}

void stopLamp(){
	lampOff();
	buzzerAlarm();
}

void startLamp(float duration){
	lampTimerIndex = timers.addTimer(duration, stopLamp);
	lampOn();

	melodyActivation.play();
}

void forceStopLamp(){
	lampOff();
	melodyForceStop.play();

	timers.stopTimer(lampTimerIndex);
}

void initServer(){
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
		Serial.println("index");
		req->send(LittleFS, "/index.html");
	});

	server.serveStatic("/static/", LittleFS, "/");
	server.begin();
}

void setup() {
	EEPROM.begin(EEPROM_SIZE);
	
	pinMode(LED, OUTPUT);
	pinMode(BUZZER, OUTPUT);
	Serial.begin(SERIAL_BAUD);

	// Set all the melodies
	float notesStart[MELODYSIZE] = {329.63, 392, 440, 523.25, -1};
	melodyStart = Melody(BUZZER, &timers, 180, notesStart);

	float notesActivation[MELODYSIZE] = {440, 261.63, 523.25, -1};
	melodyActivation = Melody(BUZZER, &timers, 180, notesActivation);

	float notesForceStop[MELODYSIZE] = {329.63, -1};
	melodyForceStop = Melody(BUZZER, &timers, 60, notesForceStop);

	float notesNetworkHost[MELODYSIZE] = {164.81, 196, -1};
	melodyNetworkHost = Melody(BUZZER, &timers, 180, notesNetworkHost);

	float notesNetworkConnect[MELODYSIZE] = {164.81, 196, -1};
	melodyNetworkConnect = Melody(BUZZER, &timers, 180, notesNetworkConnect);

	float notesNetworkInitializing[MELODYSIZE] = {329.63, 0, 329.63, -1};
	melodyNetworkInitializing = Melody(BUZZER, &timers, 180, notesNetworkInitializing);

	float notesInitError[MELODYSIZE] = {65.41, -1};
	melodyInitError = Melody(BUZZER, &timers, 30, notesInitError);

	if(!LittleFS.begin()){
		Serial.println("An error occured while mounting SPIFFS");
		melodyInitError.play();
		return;
	}

	melodyStart.play(); 

	connectToNetwork = (*(uint8_t*)(storage.get(0))) == 1;
	timers.addTimer(1000, [](){ // Wait for the absolutely necessary buzzer to finish playing
		action = CONNECT;
	});

	initServer();
}

void loop() {
	switch(action){
		case IDLE:
			break;
		case CONNECT:
			melodyNetworkInitializing.play();

			timers.addTimer(750, [](){ // We must wait for the all-important buzzer to finish playing
				if(connectToNetwork){
					Serial.print("Connecting to network");

					char (*ssid)[32];
					ssid = (char(*)[32])storage.get(2);

					char (*pass)[32];
					pass = (char(*)[32])storage.get(3);

					WiFi.begin(*ssid, *pass);
				}
				else{
					Serial.print("Creating AP...");
					WiFi.softAP(defaultSSID, defaultPassword);
				}

				action = CONNECTING;
			});

			action = IDLE;
			break;
		case CONNECTING:
			if(!connectToNetwork || (connectToNetwork && WL_CONNECTED)){
				action = CONNECTED;
			}
			break;
		case CONNECTED:
			Serial.println(" DONE");
			
			if(connectToNetwork){
				melodyNetworkConnect.play();

				Serial.print("IP: ");
				Serial.println(WiFi.localIP());
				Serial.print("Gateway: ");
				Serial.println(WiFi.gatewayIP());
				Serial.print("Subnet: ");
				Serial.println(WiFi.subnetMask());
			}
			else{
				melodyNetworkHost.play();

				Serial.print("IP: ");
				Serial.println(WiFi.softAPIP());
			}

			// initServer();

			action = IDLE;
			break;
	}

	timers.check(); // Check for expired timers
}
