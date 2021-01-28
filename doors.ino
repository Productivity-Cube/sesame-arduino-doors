#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#ifndef STASSID
#define STASSID "***"
#define STAPSK  "***"
#define DOOR_ID "bfeec271-f5b6-4d9a-8556-6ad9f83b570a"
#define SERVER "http://192.168.1.113:8000/api/"
#define API_KEY "&YmO|^7Ck!rY=nYPtR{Y?${Tzkgw0)/^UWC;>Mb9(q0=Nkq{:9GUg6E244A0Xi["
#endif

const String doorId = DOOR_ID;
const String serverUrl = SERVER;
const String apiKey = API_KEY;
const char* ssid = STASSID;
const char* password = STAPSK;

#define ledPin 13
#define sensor1 14
#define sensor2 12

ESP8266WebServer server(80);

int people = 0;
int firstSensorToComeIn = 0;
int site = 0;
String siteStr = "";
bool personCounted = false;
bool doorsOpened = false; 
int letPeopleIn = 0;
int peopleIn = 0;
int peopleOut = 0;

void handleRoot() {
  digitalWrite(ledPin, HIGH);
  server.send(200, "text/plain", "People inside : " + String(people) + ", Incomming people: " + String(letPeopleIn) + ", PeopleIn: " + String(peopleIn) + ", PeopleOut: " + String(peopleOut));
  digitalWrite(ledPin, LOW);
}

void handleDoorsOpen () {
  letPeopleIn += 1;
  server.send(200, "text/plain", "Incomming people: " + String(letPeopleIn));
  Serial.println("People I can let in: " + String(letPeopleIn));
}

void sendRequestAboutPerson (int incoming) {
  HTTPClient http;
  http.begin(serverUrl + "clients");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + apiKey);
  int httpCode = http.POST("{\"ingoing\": " + String(incoming == 1 ? "true" : "false") + ",\"doorId\": \"" + doorId + "\"}");
  String payload = http.getString();
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
 
  http.end();  //Close connection
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/person/add", handleDoorsOpen);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  delay(1);        // delay in between reads for stability
  
  server.handleClient();
  MDNS.update();
  if (letPeopleIn == 0) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
  
  int sensor1Result = digitalRead(sensor1) ? 0 : 1;
  int sensor2Result = digitalRead(sensor2) ? 0 : 1;
  int sensors = sensor1Result + sensor2Result;
  if (firstSensorToComeIn == 0 && sensors == 1) {
    firstSensorToComeIn = sensor1Result == 1 ? 1 : 2;
  } else if (sensors == 0) {
    firstSensorToComeIn = 0;
    personCounted = false;
  } else if (sensors == 2) {
    siteStr = firstSensorToComeIn == 1 ? " w prawo " : "w lewo";
    site = firstSensorToComeIn == 1 ? 1 : 2;
    if (!personCounted) {
      personCounted = true;
      if (site == 1 && letPeopleIn != 0) letPeopleIn -= 1;
      if (site == 1) { peopleIn += 1; } else { peopleOut += 1; }
      people = people + (site == 1 ? 1 : -1);
      String log = " " + String(sensor1Result) + " " + String(sensor2Result) + " " + String(firstSensorToComeIn) + " " + site + " people: " + String(people) ;
      Serial.println(log);
      sendRequestAboutPerson(site);
    }
  }
}
