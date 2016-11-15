#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MQTT_TOPIC_LAST_WILL "state/sensor/hackcenterButtons"
#define MQTT_IN_TOPIC "StarkTower/LivingRoom/Light1"

const char* ssid     = "backspace IoT";
const char* password = "--------";
const char* mqttHost = "mqtt.core.bckspc.de";

typedef struct {
  uint8_t pin;
  const char* mqttTopic;
  const char* valueUp;
  const char* valueDown;
  uint16_t debounceMs;
  Bounce *debouncer;
} t_buttonConfiguration;

t_buttonConfiguration buttons[] = {
  {D1, "sensor/button/hackcenter/0", "released", "pressed", 200, NULL},
  {D2, "sensor/button/hackcenter/1", "released", "pressed", 200, NULL},
  {D3, "sensor/button/hackcenter/2", "released", "pressed", 200, NULL},
  {D4, "sensor/button/hackcenter/3", "released", "pressed", 200, NULL}
};

WiFiClient wifiClient;
PubSubClient mqttClient;
Bounce debouncer = Bounce();

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char *payloadString = (char*) malloc(sizeof(byte) * (length +1));
  payloadString = (char*) payload; //Terminieren
  payloadString[length] = '\0';
  Serial.println(payloadString);
}

void setup() {

  WiFi.hostname("ESP-HackcenterButtons");
  WiFi.mode(WIFI_STA);

  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(mqttHost, 1883);
  mqttClient.setCallback(callback);


  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  for (uint8_t i = 0; i < ARRAY_SIZE(buttons); i++) {
     pinMode(buttons[i].pin, INPUT_PULLUP);

     buttons[i].debouncer = new Bounce();
     buttons[i].debouncer->attach(buttons[i].pin);
     buttons[i].debouncer->interval(buttons[i].debounceMs);
  }
}

int value = 0;

void connectMqtt() {

  bool newConnection = false;
  while (!mqttClient.connected()) {
    mqttClient.connect("hackcenterButtons", MQTT_TOPIC_LAST_WILL, 1, true, "disconnected");

    Serial.println("Connected");
    delay(1000);
    newConnection = true;
  }

  if(newConnection) {
    mqttClient.subscribe(MQTT_IN_TOPIC);
    mqttClient.publish(MQTT_TOPIC_LAST_WILL, "connected", true);
  }
}

void loop() {

  connectMqtt();
  mqttClient.loop();

  for (uint8_t i = 0; i < ARRAY_SIZE(buttons); i++) {
    buttons[i].debouncer->update();

    if(buttons[i].debouncer->rose()) {
      mqttClient.publish(buttons[i].mqttTopic, buttons[i].valueUp, true);
    } else if(buttons[i].debouncer->fell()) {
      mqttClient.publish(buttons[i].mqttTopic, buttons[i].valueDown, true);
    }
  }
}
