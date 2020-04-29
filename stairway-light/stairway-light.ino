// Include used libraries 
#include <ESP8266WiFi.h> 
#include <PubSubClient.h>


// Pin definition
#define LED D5
#define BUTTON D1 

// maximum received message length 
#define MAX_MSG_LEN (128)

// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "stairway-light";
const char *topic = "stairway/light";

// Instances for wifi and MQTT communication
WiFiClient espClient;
PubSubClient client(espClient);

// Connect to wifi
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
  if (strcmp(message, "set:off") == 0) {
    digitalWrite(LED, LOW);
  } else if (strcmp(message, "set:on") == 0) {
    digitalWrite(LED, HIGH);
  }
} 

void setup() {
  // Set LED pin
  pinMode(D3, OUTPUT);
  digitalWrite(D3, LOW );
  pinMode(LED, OUTPUT);      
  digitalWrite(LED, LOW);
  // Set button pin
  pinMode(BUTTON, INPUT);
  // Configure serial port for debugging
  Serial.begin(9600);
  // Initialise wifi connection - this will wait until connected
  connectWifi();
  // Connect to MQTT server  
  client.setServer(serverHostname, 1883);
  connectMQTT();
  client.publish(topic, "state:off");
  client.setCallback(callback);
}

void loop() {
    // Checking if button is pressed
    if (digitalRead(BUTTON) == HIGH)
    {
      // If button is pressed, turn on the light
      digitalWrite(LED, HIGH);
      Serial.println("Light on");
      // Send info to MQTT server
      if (!client.connected()) {
      // Reconection to MQTT server
      connectMQTT();
      }
      client.publish(topic, "state:on");
      // Wait 10 seconds
      delay(10000);
      // Turn off the light
      digitalWrite(LED, LOW);
       Serial.println("Light off");
      // Send info to MQTT server
      client.publish(topic, "state:off");
    }
    // Client loop
    client.loop();
    // Wait
    delay(100);
}
