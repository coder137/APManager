#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// ! libs
#include <cJSON.h>
#include <APManager.h>

void setup() {
    Serial.begin(9600);
    while(!Serial);

    if (!SPIFFS.begin()) {
        Serial.println("Spiffs filesystem not init");
        while(1) { delay(1000); } //get stuck here
    }

    APManager manager;

    // Add custom function here
    ESP8266WebServer& server = manager.getServerReference();
    server.on("/test", [&](){
        server.send(200, "text/plain", "Hello World");
    });

    // configure server here (contains 3 endpoints)
    manager.configureServer();
    int id = manager.autoconnect(); // Will get stuck here for a while
    if (id != WifiHelper::STARTED) {
        // TODO, Have to restart the device
        while(1) { delay(1000); }
    }
    Serial.println("WiFiIP: "+WiFi.localIP().toString());

    // LED is ON here
    pinMode(D4, OUTPUT);
    digitalWrite(D4, LOW);

    // Do other things here
}

void loop() {
}
