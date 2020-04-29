// Include used libraries 
#include <SPI.h>
#include <MFRC522.h>

#include <ESP8266WiFi.h> 
#include <PubSubClient.h>

// PIN definition
#define RST_PIN         D3          
#define SS_PIN          D4

#define LED             D0

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// definition of variables
String uuid;
boolean waiting;  

// maximum received message length 
#define MAX_MSG_LEN (128)
// Wifi configuration
const char* ssid = "IoT-WiFi";
const char* password = "Cisc0Nag2019IoT";

// MQTT Configuration
const char *serverHostname = "10.1.2.1";
const char *clientId = "garden-rfid_reader";
const char *topic = "garden/rfid_reader";
String message;

// Client instances for wifi and MQTT communication
WiFiClient espClient;
PubSubClient client(espClient);

// Read uuid from rfid chip
String getID() {
  uuid = "";
  for ( uint8_t i = 0; i < 4; i++) {  //
    uuid += String(mfrc522.uid.uidByte[i]);
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return uuid;
}

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
  if (strcmp(message, "authorized") == 0) {
    Serial.println("ok");
  } else if (strcmp(message, "unauthorized") == 0) {
    digitalWrite(LED, HIGH);
    delay(2000);
    digitalWrite(LED, LOW);
  }
  waiting = false;
}

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  waiting = false;
  Serial.begin(9600);   // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  connectWifi();
  client.setServer(serverHostname, 1883);
  client.setCallback(callback);
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (mfrc522.PICC_IsNewCardPresent()) {
    // Select one of the cards
    if (mfrc522.PICC_ReadCardSerial()) {
        uuid = getID();
        Serial.println("Scanned uuid:" + uuid);
        // Publish info
        if (!client.connected()) {
            connectMQTT();
        }
        message = String("uuid:") + uuid;
        // send uuid to sever
        client.publish(topic, message.c_str());
        // server should listen to response
        waiting = true;
        while (waiting) {
          // Wait before listening to server response
          delay(100);
          // Check for response - checking only, when waiting for authorization, this saves power
          client.loop();
        }
    }
  }
  // delay before next loop
  delay(100);
}
