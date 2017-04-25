//#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE
#define MY_RX_MESSAGE_BUFFER_FEATURE
#define MY_RF24_IRQ_PIN 2
//#define MY_SPECIAL_DEBUG
//#define MY_DEBUG_VERBOSE_RF24

#include <MyConfig.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define LIGHT 6
#define DSPIN 4   //Dallas sensor

OneWire oneWire(DSPIN);
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature;

#define CHILD_ID_TEMP 0
#define CHILD_ID_LIGHT 1
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP); 
MyMessage msgSwitch(CHILD_ID_LIGHT, V_STATUS);
MyMessage msgDimmer(CHILD_ID_LIGHT, V_PERCENTAGE);

void setup()  
{
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);
  
  // requestTemperatures() will not block current thread
  //sensors.setWaitForConversion(false);
}

void presentation()
{
  sendSketchInfo("Sensor Relay", "0.5");
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_LIGHT, S_DIMMER, "Light switch/dimmer");

}

unsigned long interval = 0;
void loop() 
{
  if (millis() - interval > 60000ul) {
    // Temperature
    // Fetch temperatures from Dallas sensors
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);
    #ifdef MY_DEBUG
      Serial.print("Temperature: ");
      Serial.println(temperature);
    #endif
    if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
      send(msgTemp.set(temperature, 1));
      lastTemperature = temperature;
    }
    interval = millis();
  }
}

int dimmerValue = 0;
void receive(const MyMessage& message)
{
  if (message.sensor == CHILD_ID_LIGHT) {
    if (message.type == V_PERCENTAGE) {
      if (message.getCommand() == C_SET) {
        dimmerValue = message.getInt();
      } else {    // C_REQ
        MyMessage replyMsg(CHILD_ID_LIGHT, V_PERCENTAGE);
        replyMsg.setDestination(message.sender);
        send(replyMsg.set(dimmerValue));
      }
    }
  }
}
