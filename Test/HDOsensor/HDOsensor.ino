#define MY_DEBUG
#define MY_RADIO_NRF24
#define CHILD_ID_HDO 1
#define CHILD_ID_POWER 2
#define HDO_PIN 3
#define POWER_PIN 2

#include <MySensors.h>  

MyMessage msgHDO(CHILD_ID_HDO, V_TRIPPED);
MyMessage msgPower(CHILD_ID_POWER, V_KWH);

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Test HDO sensor", "0.1");
  present(CHILD_ID_HDO, S_DOOR, "HDO signal");
  present(CHILD_ID_POWER, S_POWER, "Power meter");
}

void setup() {
  pinMode(HDO_PIN, INPUT);
  pinMode(POWER_PIN, INPUT);
}

void loop() {
  loop10s();
  loop60s();
}

unsigned long timer10s = 0;
void loop10s()
{
  if (millis() - timer10s > 15000) {
    timer10s = millis();
    send(msgHDO.set(digitalRead(HDO_PIN)) );
  }
}

unsigned long timer60s = 0;
void loop60s()
{
  if (millis() - timer60s > 60000) {
    timer60s = millis();
    send(msgPower.set(digitalRead(POWER_PIN)) );
  }
}

