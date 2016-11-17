#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include "config.h"

typedef struct {
  uint8_t pin;
  const char* mqttTopic;
  uint16_t debounceMs;
  Bounce *debouncer;
} t_buttonConfiguration;

typedef struct {
	const char* mqttTopic;
	uint8_t pin;
} t_ledConfiguration;

t_buttonConfiguration buttons[] = {
  {D1, MQTT_BASE_TOPIC "button/0", 200, NULL},
  {D2, MQTT_BASE_TOPIC "button/1", 200, NULL},
  {D3, MQTT_BASE_TOPIC "button/2", 200, NULL},
  {D4, MQTT_BASE_TOPIC "button/3", 200, NULL}
};

t_ledConfiguration leds[] = {
	{MQTT_BASE_TOPIC "led/0", D5},
	{MQTT_BASE_TOPIC "led/0", D6},
  {MQTT_BASE_TOPIC "led/0", D7},
  {MQTT_BASE_TOPIC "led/0", D8},
};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void switchLed(t_ledConfiguration led, char* payload) {
  if(strcmp(payload, "ON")){
    digitalWrite(led.pin, HIGH);
  }
  else if(strcmp(payload, "OFF")){
    digitalWrite(led.pin, LOW);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for(auto led : leds) {
	  if (strcmp(led.mqttTopic, topic) == 0) {
		  switchLed(led, (char*)payload);
		  break;
	  }
  }
}

void wifiSetup() {

  WiFi.hostname(HOSTNAME);
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {

  Serial.begin(115200);
  delay(10);

  wifiSetup();

  mqttClient.setServer(mqttHost, 1883);
  mqttClient.setCallback(callback);


  for (auto button : buttons) {
     pinMode(button.pin, INPUT_PULLUP);

     button.debouncer = new Bounce();
     button.debouncer->attach(button.pin);
     button.debouncer->interval(button.debounceMs);
  }
  for (auto led : leds){
    pinMode(led.pin, OUTPUT);
  }
}

void connectMqtt() {

  bool newConnection = false;
  while (!mqttClient.connected()) {
    mqttClient.connect("hackcenterButtons", MQTT_TOPIC_LAST_WILL, 1, true, "disconnected");

    Serial.println("Connected");
    delay(1000);
    newConnection = true;
  }

  if(newConnection) {
    for (auto led : leds){
      mqttClient.subscribe(led.mqttTopic);
    }
    mqttClient.publish(MQTT_TOPIC_LAST_WILL, "connected", true);
  }
}

void loop() {

  connectMqtt();
  mqttClient.loop();

  for (auto button : buttons) {
    button.debouncer->update();

    if(button.debouncer->rose()) {
      mqttClient.publish(button.mqttTopic, "pressed", true);
    } else if(button.debouncer->fell()) {
      mqttClient.publish(button.mqttTopic, "released", true);
    }
  }
}
