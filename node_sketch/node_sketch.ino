///////////////////////////////////////////////////////////////////////
//
//    Node Module in ESP8266 Network
//    Brian Costantino
//
/////////////////////////////////////////////////////////////////
//
//    Resources:
//      Station - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html
//      HttpClient - https://links2004.github.io/Arduino/dd/d8d/class_h_t_t_p_client.html
//
////////////////////////////////////////////////////////

// import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// define AP credentials
#define AP_SSID ""
#define AP_PASS ""
#define MASTER_ADDY ""

String apiKey;
StaticJsonDocument<200> currentTasks;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.printf("Connecting to %s", AP_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  initNode();
  Serial.println();
  getNodes();
  Serial.println();
  requestTask(6, false);
}

void loop() {
  getTasks();
  Serial.printf("%d task(s) pending\n", currentTasks.size());
  for (int i=0; i<currentTasks.size(); i++) {
    const char* id = currentTasks[i]["id"];
    const int type = currentTasks[i]["type"];
    const bool repeat = currentTasks[i]["repeat"];
    Serial.printf("%d: %s - type: %d - repeat? %s\n", i+1, id, type, repeat ? "true" : "false");
  }
  delay(50000);
}

void initNode() {
  String s;
  const size_t CAPACITY = JSON_OBJECT_SIZE(4);
  
  StaticJsonDocument<CAPACITY> doc;
  doc["name"] = WiFi.hostname();
  doc["ip"] = WiFi.localIP().toString();
  serializeJson(doc, s);
  
  if (postToMaster(String("/init_node"), s, apiKey)) {
    Serial.printf("Successfully synced with master, API key: %s\n", apiKey.c_str());
  } else {
    Serial.println("Failed to sync with master");
  }
}

void getNodes() {
  char s[40];
  sprintf(s, "/nodes/%s", apiKey.c_str());

  String responseBuffer;
  getFromMaster(String(s), responseBuffer);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, responseBuffer);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  Serial.printf("%d node(s) connected to master\n", doc.size());
  for (int i=0; i<doc.size(); i++) {
    const char* name = doc[i]["hostname"];
    const char* local_ip = doc[i]["local_ip"];
    Serial.printf("%d: %s (%s)\n", i+1, String(local_ip).c_str(), String(name).c_str());
  }
}

void getTasks() {
  char s[40];
  sprintf(s, "/tasks/%s", apiKey.c_str());
  String responseBuffer;
  if (getFromMaster(String(s), responseBuffer)) {
    DeserializationError error = deserializeJson(currentTasks, responseBuffer);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    } 
  }
}

void requestTask(int type, bool repeat) {
  String s;
  const size_t CAPACITY = JSON_OBJECT_SIZE(6);
  
  StaticJsonDocument<CAPACITY> doc;
  doc["type"] = type;
  doc["repeat"] = repeat;
  serializeJson(doc, s);

  String responseBuffer;
  if (postToMaster(String("/tasks"), s, responseBuffer)) {
    Serial.printf("Task created with id: %s\n", responseBuffer.c_str());
  }
}

bool postToMaster(String path, String payload, String &responseDestination) {
  if (WiFi.isConnected()) {
    WiFiClient wifi;
    HTTPClient http;
    String _uri = MASTER_ADDY+path;
    Serial.printf("POST to %s\n", _uri.c_str());
    if (http.begin(wifi, _uri)) {
      http.addHeader("Content-Type", "application/json");
      
      int statusCode = http.POST(payload);
      if (statusCode>0) {
        responseDestination = http.getString();
        return true;
      } else {
        Serial.printf("POST failed, error: %s\n", http.errorToString(statusCode).c_str());
      }
    }
  } else {
    Serial.println("POST failed, not connected");
  }
  return false;
}

bool getFromMaster(String path, String &responseDestination) {
  if (WiFi.isConnected()) {
    WiFiClient wifi;
    HTTPClient http;
    String _uri = MASTER_ADDY+path;
    Serial.printf("GET from %s\n", _uri.c_str());
    if (http.begin(wifi, _uri)) {
      
      int statusCode = http.GET();
      if (statusCode>0) {
        responseDestination = http.getString();
        return true;
      } else {
        Serial.printf("GET failed, error: %s\n", http.errorToString(statusCode).c_str());
      }
    }
  } else {
    Serial.println("GET failed, not connected");
    return false;
  }
}
