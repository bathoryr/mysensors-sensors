#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE

#include <MyConfig.h>
#include <MySensors.h>

#define CHILD_ID_LIGHT 1
#define CHILD_ID_SWITCH 2
#define SERVER_NODE 9   // ID of the "server" node

//MyMessage dimmerMsg(CHILD_ID_LIGHT, V_PERCENTAGE);


void setup() {
  // put your setup code here, to run once:
  request(CHILD_ID_LIGHT, V_PERCENTAGE);    
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  present(CHILD_ID_LIGHT, S_DIMMER);
  present(CHILD_ID_SWITCH, S_BINARY);
  sendSketchInfo("Test request value from GW", "0.1");
}

void loop() {
  // put your main code here, to run repeatedly:

}

int dimmerValue = 0;
void receive(const MyMessage &message)
{
  if (message.sensor == CHILD_ID_LIGHT) {
    if (message.type == V_PERCENTAGE) {
      if (message.getCommand() == C_SET) {
        Serial.print("Receved new value for dimmer: ");
        Serial.println(dimmerValue = message.getInt());
        if (message.sender == SERVER_NODE) {
          MyMessage dimmerMsg(CHILD_ID_LIGHT, V_PERCENTAGE);
          send(dimmerMsg.set(dimmerValue));
        }
      } else {    // C_REQ
        Serial.print("Request from other node, returning value: ");
        Serial.println(dimmerValue);
      }
    }
  }
  if (message.sensor == CHILD_ID_SWITCH) {
    if (message.type == V_STATUS) {
      Serial.print("Receved new value for switch: ");
      Serial.println(message.getBool());
      digitalWrite(LED_BUILTIN, message.getBool() ? HIGH : LOW);
      request(1, V_PERCENTAGE, SERVER_NODE);
    }
  }
  Serial.print("Sensor: ");
  Serial.print(message.sensor);
  Serial.print(", command: ");
  Serial.print(message.getCommand());
  Serial.print(", ACK: ");
  Serial.print(message.isAck() ? "yes" : "no");
  Serial.print(", type:");
  Serial.println(message.type);
}

void serialEvent()
{
  char inChar = (char)Serial.read();
  if (inChar == 'r') {
    request(CHILD_ID_LIGHT, V_PERCENTAGE);  
  }
}

