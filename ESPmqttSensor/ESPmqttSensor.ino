#include <FS.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson

const char* TOPIC_OUT = "home/ESP/out";
const char* TOPIC_IN  = "home/ESP/in";
const char* MQTT_SRV  = "10.7.1.1";
const char* CFG_FILE  = "/config.json";
char mqtt_server[40];

//flag for saving data
bool shouldSaveConfig = false;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

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

void read_file() {
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(CFG_FILE)) {
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      configFile.close();
      
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        Serial.print("Parsed json: ");
        strcpy(mqtt_server, json["mqtt_server"]);
        Serial.println(mqtt_server);
      }
    } else {
      Serial.println("No config file exists");
    }
  }
}

void write_file() {
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    json["mqtt_server"] = mqtt_server;
    File configFile = SPIFFS.open(CFG_FILE, "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    } else {
      json.prettyPrintTo(Serial);
      json.printTo(configFile);
      configFile.close();
    }
  }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    read_file();
    //set config save notify callback
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.autoConnect();

    write_file();
    // MQTT 
    client.setServer(mqtt_server, 1883);
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
    if (client.connect("WeMos_Client")) {
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

