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
#include <L298N.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);
std::list<uint32_t> client_ids = {};
L298N motor(D3, D7, D8);
int ir_sensor_pin = D6;
int old_time = 0;
int c_time;
float rev = 0;
int rpm;

void IRAM_ATTR isr() //interrupt service routine
{
rev++;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(D4,OUTPUT);
  digitalWrite(D4,HIGH);
  pinMode(D6, INPUT_PULLUP);
  attachInterrupt(D6, isr,RISING);
  motor.setSpeed(motor.getSpeed());
  connectToWiFi();
  sensors.begin();
}

void loop() {
  
  Serial.println(rev);
  motor.forward();
  c_time=millis()-old_time;
  rpm=(rev/c_time)*60000*2;
  old_time=millis();
  rev=0;
  sensors.requestTemperaturesByIndex(0);
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data shit");
  }
  else
  {
  Serial.println(tempC);
  StaticJsonDocument<200> doc;
  doc["temp"] = tempC;
  doc["speed"] = rpm;
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
  //WiFi.mode(WIFI_STA);
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
        motor.setSpeed(0);
      }
    }
    if (strcmp(action, "set_speed") == 0) {
      Serial.println("setting the speed");
      char temp_speed_array[10];
      strncpy(temp_speed_array,payload,sizeof(temp_speed_array));
      unsigned short speed = (unsigned short) strtoul(temp_speed_array, NULL, 10);
      motor.setSpeed(speed);
    }
  }
  else if(type == WS_EVT_ERROR){
    Serial.println("internal error");
  }
}
