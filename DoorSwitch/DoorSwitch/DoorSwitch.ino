//#define MY_DEBUG
#define MY_DISABLED_SERIAL
#define MY_RADIO_NRF24
#include <MyConfig.h>
#include <MySensors.h>  

#define PIN_DOOR 2
#define PIN_SWITCH 8
#define CHILD_ID_DOOR 1

MyMessage msg(CHILD_ID_DOOR, V_TRIPPED);

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_DOOR, INPUT_PULLUP);
  pinMode(PIN_SWITCH, OUTPUT);
}

void presentation() {
  sendSketchInfo("Door sensor", "0.1.0");
  present(CHILD_ID_DOOR, S_DOOR);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(sleep(0, CHANGE, 0) == 0) {
    digitalWrite(PIN_SWITCH, digitalRead(PIN_DOOR));
    send(msg.set(digitalRead(PIN_DOOR) == HIGH));
  }
}
