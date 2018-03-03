/*
 *  This sketch sends a message to a TCP server
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

const char* TOPIC_OUT = "home/ESP/out";
const char* TOPIC_IN  = "home/ESP/in";

WiFiClient espClient;
PubSubClient client(espClient);

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  } else {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED off by making the voltage HIGH
  }
  client.publish(TOPIC_OUT, "Message received");
}

void setup() {
    Serial.begin(115200);
    delay(10);
    
    pinMode(LED_BUILTIN, OUTPUT);

    // We start by connecting to a WiFi network
    setup_wifi("bathory-iot", "Cavour260");
    // MQTT 
    client.setServer("10.7.1.1", 1883);
    client.setCallback(mqtt_callback);
}

void setup_wifi(const char* ssid, const char* password) {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(TOPIC_OUT, "hello world");
      // ... and resubscribe
      client.subscribe(TOPIC_IN);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
unsigned long ms = 0L;
void loop() {
  if (millis() - ms > 2000) {
    ms = millis();
    if (!client.connected()) {
      reconnect();
    }
  }
  client.loop();
}

