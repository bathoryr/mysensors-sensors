// MYSBL, 16MHz XTal

#define MY_RADIO_NRF24
#define MY_DISABLED_SERIAL
//#define MY_REPEATER_FEATURE
//#define MY_DEBUG
#include <MySensors.h>
#include <MyConfig.h>
#include <RCSwitch.h>

#define PIN_INT_LED 3
#define PIN_LIGHT   4

#define CHILD_ID_DIMMER 1
#define CHILD_ID_LIGHT  2
#define NODE_LIVROOM_DIMMER 5

// ID's of Remote control
const unsigned long RC_ID = 3507000;     // RC1 - Living room
// Define RC button values
enum RC_buttons {RC_PWR_OFF = 201, RC_PWR_ON = 204, RC_BRIGHT_UP = 205, RC_BRIGHT_DN = 206, 
                 RC_PCNT100 = 207, RC_PCNT50 = 208, RC_PCNT25 = 209, 
                 RC_MODE_UP = 211, RC_MODE_DN = 217, RC_SPEED_UP = 215, RC_SPEED_DN = 213};

// RF 433 receiver
RCSwitch mySwitch = RCSwitch();

void setup() {
  pinMode(PIN_LIGHT, OUTPUT);
  mySwitch.enableReceive(0);                // Receiver on inerrupt 0 => that is pin #
#ifdef MY_DEBUG
  pinMode(PIN_INT_LED, OUTPUT);
  for (byte i = 0; i++; i < 5) {
    digitalWrite(PIN_INT_LED, HIGH);
    delay(200);
    digitalWrite(PIN_INT_LED, LOW);
    delay(200);
  }
#endif
}

//MyMessage msgDimmer(CHILD_ID_DIMMER, V_PERCENTAGE);
MyMessage msgLight(CHILD_ID_LIGHT, V_STATUS);

void presentation() {
  sendSketchInfo("Kid's light", "1.0.0");
  present(CHILD_ID_DIMMER, S_DIMMER, "Main light intensity");
  present(CHILD_ID_DIMMER, S_LIGHT, "Main light switch");
  present(CHILD_ID_LIGHT, S_LIGHT, "Light switch");
}

void loop() {
  GetRCSwitch();
  wait(200);

#ifdef MY_DEBUG
  if (digitalRead(PIN_INT_LED) == LOW) 
    delayHIGH();
  else
    delayLOW();
#endif
}

void receive(const MyMessage& message)
{
  switch (message.sensor) {
    case CHILD_ID_LIGHT:
      if (message.type == V_STATUS) {
        message.getBool() ? digitalWrite(PIN_LIGHT, HIGH) : digitalWrite(PIN_LIGHT, LOW);
      }
      break;
  }
}

void GetRCSwitch()
{
  if (mySwitch.available()) {
    digitalWrite(PIN_INT_LED, HIGH);
    unsigned long value = mySwitch.getReceivedValue();
    if (value != 0) {
      #ifdef MY_DEBUG
      Serial.print("Received ");
      Serial.println(value);
      #endif
      switch (value) {
        case RC_ID + RC_PWR_OFF:
        {
          MyMessage msg(CHILD_ID_DIMMER, V_STATUS);
          msg.setDestination(NODE_LIVROOM_DIMMER);
          send(msg.set(false), true);
          break;
        }
        case RC_ID + RC_PWR_ON:
        {
          MyMessage msg(CHILD_ID_DIMMER, V_STATUS);
          msg.setDestination(NODE_LIVROOM_DIMMER);
          send(msg.set(true), true);
          break;
        }
        case RC_ID + RC_PCNT100:
        {
          //send(msgDimmer.set(100));
          MyMessage msg(CHILD_ID_DIMMER, V_PERCENTAGE);
          msg.setDestination(NODE_LIVROOM_DIMMER);
          send(msg.set(100));
          break;
        }
        case RC_ID + RC_PCNT50:
        {
          //send(msgDimmer.set(50));
          MyMessage msg(CHILD_ID_DIMMER, V_PERCENTAGE);
          msg.setDestination(NODE_LIVROOM_DIMMER);
          send(msg.set(50));
          break;
        }
        case RC_ID + RC_PCNT25:
        {
          //send(msgDimmer.set(25));
          MyMessage msg(CHILD_ID_DIMMER, V_PERCENTAGE);
          msg.setDestination(NODE_LIVROOM_DIMMER);
          send(msg.set(25));
          break;
        }
        case RC_ID + RC_MODE_UP:
          digitalWrite(PIN_LIGHT, HIGH);
          send(msgLight.set(1));
          break;
        case RC_ID + RC_MODE_DN:
          digitalWrite(PIN_LIGHT, LOW);
          send(msgLight.set(0));
          break;
      }
    }
    mySwitch.resetAvailable();
    digitalWrite(PIN_INT_LED, LOW);
  }
}
#ifdef MY_DEBUG
unsigned long timer1 = 0L;
void delayHIGH() {
  if (millis() - timer1 > 5000) {
    digitalWrite(PIN_INT_LED, HIGH);
    timer1 = millis();
  }
}

void delayLOW() {
  if (millis() - timer1 > 30) {
    digitalWrite(PIN_INT_LED, LOW);
    timer1 = millis();
  }
}
#endif