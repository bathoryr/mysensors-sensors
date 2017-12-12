// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define SENSOR_ID 1
#define RC_PIN 3

#include <MySensors.h>
#include <RCSwitch.h>

// Define RC button values
enum RC_buttons {RC_PWR_OFF = 201, RC_PWR_ON = 204, RC_BRIGHT_UP = 205, RC_BRIGHT_DN = 206, 
                 RC_PCNT100 = 207, RC_PCNT50 = 208, RC_PCNT25 = 209, 
                 RC_MODE_UP = 211, RC_MODE_DN = 217, RC_SPEED_UP = 215, RC_SPEED_DN = 213};
// ID's of Remote control
const unsigned long RC_ID = 3507000;     // RC1 - Living room

// RF 433 receiver
RCSwitch mySwitch = RCSwitch();

byte i = 0;
MyMessage msgSwitch(SENSOR_ID, V_STATUS);
MyMessage msgDimmer(SENSOR_ID, V_PERCENTAGE);

void setup() {
  mySwitch.enableReceive(1);                // Receiver on inerrupt 0 => that is pin #2
}

void presentation() {
  sendSketchInfo("RC control", "0.1");
  present(SENSOR_ID, S_LIGHT, "Light switch/dimmer");
}

void loop() {
  GetRCSwitch();
  loop2s();
}

unsigned long timer2s = millis();
void loop2s() {
  if (millis() - timer2s > 5000) {
    timer2s = millis();
    //send(msg.set(i++));
  }
}

void GetRCSwitch()
{
  if (mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    if (value != 0) {
      #ifdef MY_DEBUG
      Serial.print("Received ");
      Serial.println(value);
      #endif
      switch (value) {
        case RC_ID + RC_PWR_OFF:
          send(msgSwitch.set("OFF"));
          break;
        case RC_ID + RC_PWR_ON:
          send(msgSwitch.set("ON"));
          break;
        case RC_ID + RC_BRIGHT_UP:
          break;
        case RC_ID + RC_BRIGHT_DN:
          break;
        case RC_ID + RC_PCNT100:
          send(msgDimmer.set(100));
          break;
        case RC_ID + RC_PCNT50:
          send(msgDimmer.set(50));
          break;
        case RC_ID + RC_PCNT25:
          send(msgDimmer.set(25));
          break;
      }
    }
    mySwitch.resetAvailable();
  }
}


