#include "DHTesp.h"
#include "PubSubClient.h"
#include <WiFi.h>
#include <ArduinoJson.h>
const char * MQTTServer = "broker.mqttdashboard.com";
const char * MQTT_Topic = "dht22/vlute/abc/19004184";
const char * MQTT_Topicget = "dht22/vlute/get/19004184";
const int DHT_PIN = 15;
const char * MQTT_ID = "";
//
const int minSurvive = 15; // minimum level for idle, below is outfire
const int idleLow = 20; // lowest reading for healthy idle
const int idleTarget = 30; // target reading for resting idle
const int firingLow = 70; // lowest reading for actively firing
const int firingHigh = 90; // reading for full firing
int flamelevel=0;
int Port = 1883;
DHTesp dhtSensor;
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);
//
float t,h;
int ledPin = 13;
WiFiClient espClient;
PubSubClient client(espClient);
void WIFIConnect() {
  Serial.println("Connecting to SSID: Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected");
  Serial.print(", IP address: ");
  Serial.println(WiFi.localIP());
 
}
void MQTT_Reconnect() {
  while (!client.connected()) {
    if (client.connect(MQTT_ID)) {
      Serial.print("MQTT Topic: ");
      Serial.print(MQTT_Topic);
      Serial.print(" connected");
      client.subscribe(MQTT_Topicget);
      Serial.println("");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  } 
}


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String stMessage;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    stMessage += (char)message[i];
    if (stMessage == "on") {
    digitalWrite(ledPin, HIGH);
    delay(400);
    digitalWrite(ledPin, LOW);
    tone(23,250);
    delay(400);
    noTone(23);
  }
  else if (stMessage == "off") {
    noTone(23);
    digitalWrite(ledPin, LOW);
    
  }
  }
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  WIFIConnect();
  LCD.init();
  LCD.backlight();
  client.setServer(MQTTServer, Port);
  client.setCallback(callback);
  pinMode(ledPin, OUTPUT);
  pinMode(12, INPUT);
  pinMode(23, OUTPUT);
  LCD.clear();
  LCD.setCursor(0,0);
}
void toJsonObject(char * outputBuffer, size_t bufSize) {
  uint8_t fgPalette = 185;
  StaticJsonDocument<200> doc;
  JsonObject object = doc.to<JsonObject>();
  object["t"] = t;
  object["h"] = h;
  object["flame"] = flamelevel;
  serializeJson(doc, outputBuffer, bufSize);
}
void loop() {
  delay(10);
  if (!client.connected()) {
    MQTT_Reconnect();
  }
  client.loop();
  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  //Serial.println("Temp: " + String(data.temperature, 2) + "°C");
  t=data.temperature;
  h=data.humidity;
  char buffer[100];
  toJsonObject(buffer, sizeof(buffer));
  client.publish(MQTT_Topic, String(buffer).c_str());
  float analogValue = analogRead(33);
  flamelevel = map(analogValue, 0, 1024, 100, 0);
  LCD.setCursor(0, 0);
  LCD.print(F("Flame: "));
  if (flamelevel >= firingHigh) { // stoker is fully firing
    LCD.print("Full Fire");
  }

  if ((flamelevel >= firingLow) && (flamelevel < firingHigh)) { // stoker is firing
    LCD.print("Firing   ");
  }

  if ((flamelevel < firingLow) && (flamelevel > idleLow) ) {  // idle fire
    LCD.print("Idle fire ");
  }

  if ((flamelevel <= idleLow) && (flamelevel >= minSurvive) ) {  // low fire
    LCD.print("Low fire  ");
  }

  if (flamelevel < minSurvive) {  // fire out
    LCD.print("FIRE OUT! ");
    // send alert
  }
  delay(200);
}

