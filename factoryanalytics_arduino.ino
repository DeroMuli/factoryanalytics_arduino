#include <ESP8266WiFi.h>
#include "secret_file.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
int pinState = LOW;
OneWire oneWire(D5);
DallasTemperature sensors(&oneWire);

void setup() {
  // put your setup code here, to run once:
  pinMode(D4,OUTPUT);
  connectToWiFi();
  sensors.begin();
}

void loop() {
    // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if (tempC != DEVICE_DISCONNECTED_C)
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
  }
  else
  {
    Serial.println("Error: Could not read temperature data");
  }
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["temp"] = 34;
  jsonDoc["speed"] = 125;
  String jsonData;
  serializeJson(jsonDoc, jsonData);
  ws.textAll(jsonData);
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
   WiFi.begin(getWifiSsid(), getWifiPassword());
   int retries = 0;
while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) {
   retries++;
   delay(500);
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
    server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request){
      pinState = !pinState;
      digitalWrite(D4, pinState); 
      StaticJsonDocument<200> doc;
      doc["state"] = pinState;
      String jsonStr;
      serializeJson(doc, jsonStr);
      request->send(200,"application/json", jsonStr);
    });
    server.begin();
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {
    Serial.printf("Websocket client #%u connected\n", client->id());
  } else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("Websocket client #%u disconnected\n", client->id());
  } else if(type == WS_EVT_DATA) {
    Serial.printf("Websocket client #%u data received: ", client->id());
    for(size_t i=0; i<len; i++) {
      Serial.print((char) data[i]);
    }
    Serial.println();
  }
  else if(type == WS_EVT_ERROR){
    Serial.println("internal error");
  }
}
