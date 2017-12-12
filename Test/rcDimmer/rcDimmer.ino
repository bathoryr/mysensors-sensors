// Enable debug prints
#define MY_DEBUG
//#define MY_NODE_ID 33

#include <MySensors.h>
#include <MyConfig.h>
#include <RCSwitch.h>

// ID's of Remote control
const unsigned long RC_ID = 3507000;     // RC1 - Living room
// Define RC button values
enum RC_buttons {RC_PWR_OFF = 201, RC_PWR_ON = 204, RC_BRIGHT_UP = 205, RC_BRIGHT_DN = 206, 
                 RC_PCNT100 = 207, RC_PCNT50 = 208, RC_PCNT25 = 209, 
                 RC_MODE_UP = 211, RC_MODE_DN = 217, RC_SPEED_UP = 215, RC_SPEED_DN = 213};

#define CHILD_ID_LIGHT 8

MyMessage switchMsg(CHILD_ID_LIGHT, V_STATUS);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);

byte LastDimValue;
// RF 433 receiver
RCSwitch mySwitch = RCSwitch();

void setup() {
  // put your setup code here, to run once:
  mySwitch.enableReceive(0);                // Receiver on inerrupt 0 => that is pin #2
}

void loop() {
  // put your main code here, to run repeatedly:
  GetRCSwitch();
}

void GetRCSwitch()
{
  if (mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    if (value != 0) {
      byte dimValue = LastDimValue;
      Serial.print("Received ");
      Serial.println(value);
      Serial.print("LastDimValue=");
      Serial.println(LastDimValue);
      switch (value) {
        case RC_ID + RC_PWR_OFF:
          break;
        case RC_ID + RC_PWR_ON:
          break;
        case RC_ID + RC_BRIGHT_UP:
          dimValue = min(++dimValue, 100);
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_BRIGHT_DN:
          dimValue = max(--dimValue, 1);
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT100:
          dimValue = 100;
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT50:
          dimValue = 50;
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT25:
          dimValue = 25;
          SendDimValue(dimValue);
          break;
      }
      LastDimValue = dimValue;
    }
    mySwitch.resetAvailable();
  }
}

void SendDimValue(int dim)
{
  send(dimmerMsg.set(dim));
}
