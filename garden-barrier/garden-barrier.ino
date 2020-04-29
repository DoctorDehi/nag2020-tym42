// Include used libraries 
#include <Servo.h> 

#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

// Pin definition
int servoPin = D4; 
#define button D1
#define object_sensor D7
// Create a servo object 
Servo Servo1; 

// maximum received message length 
#define MAX_MSG_LEN (128)
// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "garden-barrier";
const char *topic = "garden/barrier";
String message;

// Client instances for wifi and MQTT communication
WiFiClient espClient;
PubSubClient client(espClient);

// Function for opening the barrier
void open_barrier() {
   Servo1.write(0); 
   client.publish(topic, "state:opened");
}

// Function for closing the barrier
void close_barrier() {
   Servo1.write(90);
   client.publish(topic, "state:closed"); 
}

void barrier_cycle() {
  open_barrier();
  Serial.println("Barrier opened");
  delay(10000);
  while (digitalRead(object_sensor) == LOW) {
    delay(100);
  }
  close_barrier();
  Serial.println("Barrier closed");
}

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
  if (strcmp(message, "set:open") == 0) {
    barrier_cycle();
  }
}

void setup() { 
   Serial.begin(9600); 
   Servo1.attach(servoPin);
   pinMode(object_sensor, INPUT);
   connectWifi();
   client.setServer(serverHostname, 1883);
   client.setCallback(callback);
   close_barrier();
   connectMQTT() ;
   client.publish(topic, "state:closed");
}

void loop(){
  if (digitalRead(button) == HIGH) {
    barrier_cycle();
  }
  // check connection to MQTT server 
  if (!client.connected()) {
    connectMQTT();
  }
  // Client loop
  client.loop();
  // Wait
  delay(500);
}
