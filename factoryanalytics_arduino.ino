#include <ESP8266WiFi.h>
#include "secret_file.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <string>
#include <iostream>
#include <list>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
std::list<uint32_t> client_ids = {};

void setup() {
  // put your setup code here, to run once:
  pinMode(D4,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
  digitalWrite(D7,HIGH);
  digitalWrite(D8,LOW);
  digitalWrite(D4,HIGH);
  connectToWiFi();
  sensors.begin();
}

void loop() {
  for (int i = 50; i <= 100; i++) {
    analogWrite(D3, i); // Set PWM value for motor A
    delay(10); // Wait for 10 milliseconds
  }
    // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  //Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  //Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
  }
  else
  {
  StaticJsonDocument<200> doc;
  doc["temp"] = tempC;
  doc["speed"] = 125;
  String jsonData;
  serializeJson(doc, jsonData);
  for (auto itr = client_ids.begin(); itr != client_ids.end(); itr++) {
    ws.text(*itr,jsonData);
   }  
  }
  delay(1000);
}

/*
* Connect your controller to WiFi
*/
void connectToWiFi() {
//Connect to WiFi Network
   Serial.begin(9600);
   Serial.println();
   Serial.println();
   Serial.print("Connecting to WiFi");
   Serial.println("...");
   WiFi.mode(WIFI_STA);
   WiFi.begin(getWifiSsid(),getWifiPassword());
   int retries = 0;
   while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) {
     Serial.println(retries);
     retries++;
     delay(1000);
     Serial.print(".");
    }
  if (retries > 14) {
    Serial.println(F("WiFi connection FAILED"));
  }
if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
    Serial.println(F("Setup ready"));
      // Start the WebSocket server
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
      // Start the HTTP server
    server.begin();
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    Serial.printf("Websocket client #%u connected\n", client->id());
  } else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("Websocket client #%u disconnected\n", client->id());
    client_ids.remove(client->id());
  } else if(type == WS_EVT_DATA) {
    Serial.println("here");
    const uint8_t size = JSON_OBJECT_SIZE(2);
    StaticJsonDocument<size> json;
    DeserializationError err = deserializeJson(json, data);
    if(err){
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
      return;
    }
    const char *action = json["action"];
    const char *payload = json["payload"];
    if (strcmp(action, "get_data") == 0) {
      Serial.println("get data");
      client_ids.push_front(client->id());
    }
    if (strcmp(action, "toggle") == 0) {
      Serial.println("switch on and off");
      if (strcmp(payload, "ON") == 0) {
        digitalWrite(D4,LOW);
      }
      else {
        digitalWrite(D4,HIGH);
      }
    }
  }
  else if(type == WS_EVT_ERROR){
    Serial.println("internal error");
  }
}
