#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Declarations
#include "TimerSet.h"
#include "Melody.h" // Buzzer melodies are important. I swear.
#include "Storage.h"

#define LED 14
#define RSTBTN 13
#define BUZZER 12
#define BUZZER_TIMES 5
#define BUZZER_LENGTH 500
#define BUZZER_FREQ 440
#define EEPROM_SIZE 128
#define SERIAL_BAUD 115200
#define TIMEOUT_ATTEMPTS 3
#define TIMEOUT_TIME 10000
#define RST_TIME 5000 // How long you need to hold down the button to reset the network settings

enum Action{
	IDLE,
	CONNECT,
	CONNECTING,
	CONNECTED
} action;

enum State{
	INACTIVE, // aka lamp off
	ACTIVE // aka lamp on
} state;

char defaultSSID[32] = "UVBox";
char defaultPassword[32] = "securepassword";

TimerSet timers;
int lampTimerIndex;
bool connectToNetwork;
bool setStaticIP;
uint8_t connectionAttempts = 0;
unsigned long rstBtnEndTime = 0;

AsyncWebServer server(80);

// EEPROM storage manager

// -------- SCHEMA --------
// 0: bool connectToNetwork
// 1: bool staticIP
// 2: char[32] targetSSID
// 3: char[32] APPassword
// 4: uint8_t[4] localIP octets (Set if staticIP == 1) 
// 5: uint8_t[4] GatewayIP octets (Set if staticIP == 1)
// 6: uint8_t[4] subnet octets (Set if staticIP == 1)
// ------------------------

size_t storageSchema[] = { sizeof(uint8_t), sizeof(uint8_t), sizeof(uint8_t) * 32, sizeof(uint8_t) * 32, 4, 4, 4 }; 
Storage storage(storageSchema, 7);

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
	state = INACTIVE;
	buzzerAlarm();
}

void startLamp(int duration){
	lampTimerIndex = timers.addTimer(duration, stopLamp);
	lampOn();
	state = ACTIVE;

	melodyActivation.play();
}

void forceStopLamp(){
	lampOff();
	melodyForceStop.play();
	state = INACTIVE;

	timers.stopTimer(lampTimerIndex);
}

String serverProcessor(const String& var){
	// afaik String doesn't allow switch statements :(

	if(var == "IPADDR"){
		if(connectToNetwork){
			return WiFi.localIP().toString();
		}
		else{
			return WiFi.softAPIP().toString();
		}
	}
	else if(var == "GATEWAY"){
		if(connectToNetwork){
			return WiFi.gatewayIP().toString();
		}
		else{
			return F(" - ");
		}
	}
	else if(var == "SUBNET"){
		if(connectToNetwork){
			return WiFi.subnetMask().toString();
		}
		else{
			return F(" - ");
		}
	}
	else if(var == "SETTINGS_STATIC_CHECKED"){
		if(setStaticIP){
			return F("checked");
		}
		else{
			return F("");
		}
	}
	else if(var == "SETTINGS_AP_CHECKED"){
		if(connectToNetwork){
			return F("");
		}
		else{
			return F("checked");
		}
	}
	else if(var == "SSID"){
		if(connectToNetwork){
			return WiFi.SSID();
		}
		else{
			return defaultSSID;
		}
	}

	return String();
}

void initServer(){
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
		req->send(LittleFS, "/index.html", String(), false, serverProcessor);
	});

	server.on("/start", HTTP_POST, [](AsyncWebServerRequest* req){
		AsyncWebParameter* p = req->getParam("time", true);
		int duration = p->value().toInt();
		startLamp(duration * 1000); // Convert to ms
		req->send(200);
	});

	server.on("/stop", HTTP_POST, [](AsyncWebServerRequest* req){
		forceStopLamp();
		req->send(200);
	});

	server.on("/getState", HTTP_POST, [](AsyncWebServerRequest* req){
		StaticJsonDocument<32> doc;

		if(state == INACTIVE){
			doc["state"] = "inactive";
		}
		else{
			doc["state"] = "active";
			doc["time"] = (int) (timers.timeRemaining(lampTimerIndex) / 1000);
		}

		char data[32];
		serializeJson(doc, data);

		req->send(200, "text/json", data);
	});

	server.on("/updateSettings", HTTP_POST, [](AsyncWebServerRequest* req){
		String data = req->getParam("data", true)->value();
		DynamicJsonDocument doc(512);
		
		DeserializationError err = deserializeJson(doc, data);

		if(err != DeserializationError::Ok){
			req->send(500);
			Serial.print("Deserialization error - ");
			Serial.println(err.c_str());
			Serial.println(data);
			return;
		}

		uint8_t network = doc["networkType"];
		storage.set(0, &network);
		Serial.print("network: ");
		Serial.println(network);

		if(network == 1){
			if(doc["networkSelected"]){
				String ssidStr = doc["selectedNetwork"]["ssid"];
				String passStr = doc["selectedNetwork"]["pass"];

				const char* ssidPtr = ssidStr.c_str();
				const char* passPtr = passStr.c_str();

				storage.set(2, (void*)ssidPtr);
				storage.set(3, (void*)passPtr);
			}
			else if(!connectToNetwork){
				Serial.println("welp no new network was selected. rip");
			}
		}

		uint8_t staticIP = doc["staticIP"];
		if(network == 1) {
			storage.set(1, &staticIP);
		}
		else{
			uint8_t val = 0;
			storage.set(1, &val);
		} 

		if(staticIP == 1){
			JsonArray ipOct = doc["staticIPData"]["localIP"];
			JsonArray gatewayOct = doc["staticIPData"]["gateway"];
			JsonArray subnetOct = doc["staticIPData"]["subnet"];

			uint8_t ip[4] = {ipOct[0], ipOct[1], ipOct[2], ipOct[3]};
			uint8_t gateway[4] = {gatewayOct[0], gatewayOct[1], gatewayOct[2], gatewayOct[3]};
			uint8_t subnet[4] = {subnetOct[0], subnetOct[1], subnetOct[2], subnetOct[3]};

			storage.set(4, &ip);
			storage.set(5, &gateway);
			storage.set(6, &subnet);
		}

		req->send(200);
		ESP.restart();
	});

	server.on("/networks", HTTP_POST, [](AsyncWebServerRequest* req){
		// For some unexplainable reason WiFi.scanNetworks(async=false) doesn't work and instantly returns 0 networks found. 
		// WiFi.scanNetworks(async=true) with WiFi.scanComplete() makes my ESP crash.
		// WiFi.scanNetworksAsync(), on the other hand, actually fucking works.
		WiFi.scanNetworksAsync([req](int n){
			Serial.print("Networks: ");
			Serial.println(n);

			if(n <= 0){
				req->send(200, "text/json", F("[]"));
				return;
			}

			DynamicJsonDocument doc(256);
			JsonArray arr = doc.to<JsonArray>();

			for(int i = 0; i < n; i++){
				DynamicJsonDocument objDoc(64);
				
				JsonObject obj = objDoc.to<JsonObject>();
				obj["ssid"] = WiFi.SSID(i);
				obj["rssi"] = WiFi.RSSI(i);

				arr.add(obj);
			}

			char data[256];
			serializeJson(doc, data);

			req->send(200, "text/json", data);
		});
	});

	server.serveStatic("/static/", LittleFS, "/");
	server.begin();
}

void connectTimeoutHandler(){
	if(action == CONNECTING){ 
		if(++connectionAttempts == TIMEOUT_ATTEMPTS){
			uint8_t val = 0;
			storage.set(0, &val);
			ESP.restart();
		}
		else{
			WiFi.disconnect();
			action = CONNECT;
		}
	}
}

void resetNetworkSettings(){
	storage.clearAll();

	ESP.restart();
}

ICACHE_RAM_ATTR void resetBtnHandler(){
	if(digitalRead(RSTBTN) == 1){
		if(rstBtnEndTime == 0) {
			rstBtnEndTime = millis() + RST_TIME;
		}
	}
	else{
		if(millis() >= rstBtnEndTime){
			resetNetworkSettings();
		}

		rstBtnEndTime = 0;
	}
}

void setup() {
	WiFi.softAPdisconnect();
	WiFi.disconnect();
	WiFi.mode(WIFI_STA);

	EEPROM.begin(EEPROM_SIZE);
	
	pinMode(LED, OUTPUT);
	pinMode(BUZZER, OUTPUT);
	pinMode(RSTBTN, INPUT);
	attachInterrupt(digitalPinToInterrupt(RSTBTN), resetBtnHandler, CHANGE);
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

	float notesNetworkConnect[MELODYSIZE] = {261.63, 220, 293.66, 329.63, -1}; // C4 A3 D4 E4
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

	uint8_t connectBuf[1];
	uint8_t staticIPBuf[1];

	storage.get(0, connectBuf);
	storage.get(1, staticIPBuf);

	connectToNetwork = connectBuf[0] == 1;
	setStaticIP = staticIPBuf[0] == 1;

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
					Serial.println("Connecting to network");

					uint8_t ssidBuf[32];
					uint8_t passBuf[32];

					storage.get(2, ssidBuf);
					storage.get(3, passBuf);

					char* ssidPtr = (char*)ssidBuf;
					char* passPtr = (char*)passBuf;

					Serial.print("SSID: ");
					Serial.println(ssidPtr);

					Serial.print("Password: ");
					Serial.println(passPtr);

					WiFi.begin(ssidPtr, passPtr);
				}
				else{
					Serial.print("Creating AP...");
					WiFi.softAP(defaultSSID, defaultPassword);
				}

				action = CONNECTING;
				timers.addTimer(TIMEOUT_TIME, connectTimeoutHandler);
			});

			action = IDLE;
			break;
		case CONNECTING:
			if(!connectToNetwork || (connectToNetwork && WiFi.status() == WL_CONNECTED)){
				action = CONNECTED;
			}
			break;
		case CONNECTED:
			Serial.println("DONE");
			
			if(connectToNetwork){
				melodyNetworkConnect.play();

				if(setStaticIP){
					uint8_t ipBuf[4];
					uint8_t gatewayBuf[4];
					uint8_t subnetBuf[4];

					storage.get(4, ipBuf);
					storage.get(5, gatewayBuf);
					storage.get(6, subnetBuf);

					IPAddress ip(ipBuf);
					IPAddress gateway(gatewayBuf);
					IPAddress subnet(subnetBuf);

					if(!WiFi.config(ip, gateway, subnet)){
						Serial.println("STA failed to configure");
						uint8_t val = 0;
						storage.set(1, &val); // If it fails to configure, set staticIP to false
					}
				}

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

			action = IDLE;
			break;
	}

	if(rstBtnEndTime != 0 && millis() >= rstBtnEndTime){
		resetNetworkSettings();
	}

	timers.check(); // Check for expired timers
}
