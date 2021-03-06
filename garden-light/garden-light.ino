// Include used libraries 
#include <ESP8266WiFi.h> 
#include <PubSubClient.h>


// Pin definition
#define LED D0

// maximum received message length 
#define MAX_MSG_LEN (128)

// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "garden-light";
const char *topic = "garden/light";

// Client instances for wifi and MQTT communication
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
      // subscribe the topic
      client.subscribe(topic);
    } else {
      Serial.printf("MQTT failed, state %s, retrying...\n", client.state());
      // Wait before retrying
      delay(2500);
    }
  }
}

// This is called whenever a message is recieved
void callback(char *msgTopic, byte *msgPayload, unsigned int msgLength) {
  // copy payload to a static string
  static char message[MAX_MSG_LEN+1];
  if (msgLength > MAX_MSG_LEN) {
    msgLength = MAX_MSG_LEN;
  }
  strncpy(message, (char *)msgPayload, msgLength);
  message[msgLength] = '\0';
  
  Serial.printf("topic %s, message received: %s\n", msgTopic, message);

  // decode message
  if (strcmp(message, "state:off") == 0) {
    digitalWrite(LED, LOW);
  } else if (strcmp(message, "state:on") == 0) {
    digitalWrite(LED, HIGH);
  }
}

void setup() {
  // Set LED pin as output
  pinMode(LED, OUTPUT);      
  digitalWrite(LED, LOW);
  // Configure serial port for debugging
  Serial.begin(9600);
  // Initialise wifi connection - this will wait until connected
  connectWifi();
  // connect to MQTT server  
  client.setServer(serverHostname, 1883);
  client.setCallback(callback);
}

void loop() {
    // check connection to MQTT server
    if (!client.connected()) {
      connectMQTT();
    }
    // Client loop
    client.loop();
    // Wait
    delay(2000);
}
