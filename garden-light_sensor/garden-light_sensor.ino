// Include used libraries 
#include <Wire.h>
#include <BH1750.h>

#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

// Light sensor instance
BH1750 lightSensor(0x23);

// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "garden-light_sensor";
const char *topic = "garden/light_sensor";
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
  Serial.begin(9600);
  Wire.begin(D2, D1);
  lightSensor.begin();
  // Initialise wifi connection - this will wait until connected
  connectWifi();
  client.setServer(serverHostname, 1883);
}

void loop() {
  // Connect to MQTT server  
  if (!client.connected()) {
      connectMQTT();
  }
  // Send light level to MQTT server
  message = String("light-level:") + String(lightSensor.readLightLevel());
  Serial.println(message);
  client.publish(topic, message.c_str()); 
  // Wait
  delay(60000);

}
