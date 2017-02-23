//#define MY_DEBUG
#define MY_RADIO_NRF24
#include <MySensors.h>

#define GAS_SENSOR_PIN  2
#define LED_COUNTER_PIN 6

#define CHILD_ID_PULSE_COUNTER 1
MyMessage msgPulseCount(CHILD_ID_PULSE_COUNTER, V_VAR1);

unsigned long pulseCounter = 0ul;   // volatile if used in ISR
unsigned long lastPC = 0ul;
byte INT_PIN;
bool countReceived = false;
byte numReq = 0;

void setup()
{
    request(CHILD_ID_PULSE_COUNTER, V_VAR1);
    pinMode(LED_COUNTER_PIN, OUTPUT);
    digitalWrite(LED_COUNTER_PIN, HIGH);
    wait(200);
    digitalWrite(LED_COUNTER_PIN, LOW);

    pinMode(GAS_SENSOR_PIN, INPUT);
    INT_PIN = digitalPinToInterrupt(GAS_SENSOR_PIN);
//    attachInterrupt(INT_PIN, onPulse, RISING);
}

void presentation()
{
    sendSketchInfo("Gas meter sensor", "0.2");
    present(CHILD_ID_PULSE_COUNTER, S_CUSTOM, "Pulse count");
}

void loop()
{
    int8_t result = sleep(INT_PIN, RISING, 59800);
    if (result == INT_PIN) {
        // When sleeping, no ISR is called, so increment counter here
        pulseCounter++;
        digitalWrite(LED_COUNTER_PIN, HIGH);
        wait(20);
        digitalWrite(LED_COUNTER_PIN, LOW);
       #ifdef MY_DEBUG
        Serial.print("Pulse count:");
        Serial.println(pulseCounter);
       #endif
    } else
    if (result < 0) {
        // Call only on timer event
        sendState();
        sendHeartbeat();
        wait(200);
    }
}

void sendState()
{
  if (countReceived == true || numReq > 10) {
    if (pulseCounter != lastPC) {
      send(msgPulseCount.set(pulseCounter));
      lastPC = pulseCounter;
    }
  } else {
    request(CHILD_ID_PULSE_COUNTER, V_VAR1);
    if (numReq <= 10);
      numReq++;
  }
}

void receive(const MyMessage &message) {
  if (message.sensor == CHILD_ID_PULSE_COUNTER) {
    if (message.type == V_VAR1) {
      pulseCounter = message.getLong();
      countReceived = true;
     #ifdef MY_DEBUG
      Serial.print("Received counter value:");
      Serial.println(pulseCounter);
     #endif
    }
  }
}

void onPulse()
{
    pulseCounter++;
}

void blinkLED()
{
    for(unsigned long i = 0; i < 3; i++) {
        digitalWrite(LED_COUNTER_PIN, HIGH);
        wait(5);
        digitalWrite(LED_COUNTER_PIN, LOW);
        wait(50);
    }
    Serial.print("Pulse count: ");
    Serial.println(pulseCounter);
}
