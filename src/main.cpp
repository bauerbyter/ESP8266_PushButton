#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include <stdio.h>
#include <Bounce2.h>

 #define BUTTON_PIN 14
 #define LED_PIN 5

// Update these with values suitable for your network.
const char* ssid = "Freifunk Backspace";
//const char* password = "S1ranevada2";
const char* mqtt_server = "192.168.2.199";    //Rasp-Server
const char* outTopic = "StarkTower/LivingRoom/Temp";
//const char* inTopic = "StarkTower/LivingRoom/Light1";

WiFiClient espClient;
PubSubClient mqttClient(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
unsigned long setOnTimestamp = 0;
unsigned long lastBlinkTimestamp = 0;
bool ledBlink = false;

Bounce debouncer = Bounce();

void setup_wifi() {
        delay(10);
        digitalWrite(LED_PIN, HIGH);
        // We start by connecting to a WiFi network
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println(ssid);

        WiFi.begin(ssid);


        while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
        }
        digitalWrite(LED_PIN, LOW);

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
}

void reconnect() {
        // Loop until we're reconnected
        while (!mqttClient.connected()) {
                Serial.print("Attempting MQTT connection...");
                digitalWrite(LED_PIN, HIGH);
                // Attempt to connect
                if (mqttClient.connect("ESP8266Client")) {
                        Serial.println("connected");
                        // Once connected, publish an announcement...
                        mqttClient.publish(outTopic, "hello");
                        // ... and resubscribe
                        //mqttClient.subscribe(inTopic);
                        digitalWrite(LED_PIN, LOW);
                } else {
                        Serial.print("failed, rc=");
                        Serial.print(mqttClient.state());
                        Serial.println(" try again in 5 seconds");
                        // Wait 5 seconds before retrying
                        delay(5000);
                }
        }
}

/*void callback(char* topic, byte* payload, unsigned int length) {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");

        char *payloadString = (char*) malloc(sizeof(byte) * (length +1));
        payloadString = (char*) payload; //Terminieren
        payloadString[length] = '\0';
        Serial.println(payloadString);
   }*/

void setup() {
        Serial.begin(115200);
        pinMode(BUTTON_PIN, INPUT_PULLUP);
        pinMode(LED_PIN, OUTPUT);
        setup_wifi();
        mqttClient.setServer(mqtt_server, 1883);
        //mqttClient.setCallback(callback);

        debouncer.attach(BUTTON_PIN);
        debouncer.interval(100);
}

void loop() {

        reconnect();
        debouncer.update();
        mqttClient.loop();
        unsigned long currentMillis = millis();
        if(debouncer.fell() ) {
                ledBlink = true;
                setOnTimestamp = millis();
                lastBlinkTimestamp = millis();
                digitalWrite(LED_PIN, HIGH);
                mqttClient.publish(outTopic, "push");
        }
        if(ledBlink) {
                if ( currentMillis - setOnTimestamp >= 4000) {
                        ledBlink = false;
                }
                else if( currentMillis - lastBlinkTimestamp >=400) {
                        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                        lastBlinkTimestamp = millis();
                }

        }
}
