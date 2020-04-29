// Include used libraries 
#include <Wire.h>
#include <BH1750.h>
#include "DHT.h"

#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

// Pin definitions
#define dht_dpin D6
#define DHTTYPE DHT11

// Light sensor instance
BH1750 lightSensor(0x23);
// Humidity and temperature sensor instance
DHT dht(dht_dpin, DHTTYPE); 

// Variables for measured values
float light_level;
float humidity;
float temperature;

// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "garden-meteostation";
const char *topic = "garden/meteostation";
String message;

// Instances for wifi and MQTT communication
WiFiClient espClient;
PubSubClient client(espClient);

// connect to wifi
void connectWifi() {
  delay(10);
  // Connecting to a WiFi network
  Serial.printf("\nConnecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected on IP address ");
  Serial.println(WiFi.localIP());
}


// connect to MQTT server
void connectMQTT() {
  // Wait until we're connected
  while (!client.connected()) {
    Serial.printf("MQTT connecting as client %s...\n", clientId);
    // Attempt to connect
    if (client.connect(clientId)) {
      Serial.println("MQTT connected");
      // ... and resubscribe
      client.subscribe(topic);
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
      // Wait before retrying
      delay(2500);
    }
  }
}

void setup() {
  // Begin serial communication    
  Serial.begin(9600);
  // Init BH1750 on i2c
  Wire.begin(D2, D1);
  lightSensor.begin();
  // Init DHT11 
  dht.begin();
  // Initialise wifi connection - this will wait until connected
  connectWifi();
  client.setServer(serverHostname, 1883);
  delay(500);
}

void loop() {
  // Connect to MQTT server  
  if (!client.connected()) {
      connectMQTT();
  }
  // Measure values
  light_level = lightSensor.readLightLevel();
  humidity = dht.readHumidity();
  temperature = dht.readTemperature(); 
  // Send data to MQTT server
  message = String("light-level:") + String(light_level);
  Serial.println(message);
  client.publish(topic, message.c_str()); 
  message = String("humidity:") + String(humidity);
  Serial.println(message);
  client.publish(topic, message.c_str()); 
  message = String("temperature:") + String(temperature);
  Serial.println(message);
  client.publish(topic, message.c_str()); 
  // Wait
  delay(60000);

}
