//#define MY_DEBUG
#define MY_RADIO_NRF24
#include <MySensors.h>

#define ulong unsigned long
#define GAS_SENSOR_PIN  2
#define LED_COUNTER_PIN 6

#define CHILD_ID_PULSE_COUNTER 1
#define CHILD_ID_VOLTAGE 2
MyMessage msgPulseCount(CHILD_ID_PULSE_COUNTER, V_WATT);
MyMessage msgLitrePM(CHILD_ID_PULSE_COUNTER, V_KWH);

volatile unsigned long pulseCounter = 0ul;   // volatile if used in ISR
unsigned long lastPulseTime = millis();
float litresPerMinute;
unsigned long lastPC = 0ul;
byte INT_PIN;
bool countReceived = false;

void setup()
{
    request(CHILD_ID_PULSE_COUNTER, V_WATT);
    pinMode(LED_COUNTER_PIN, OUTPUT);
    digitalWrite(LED_COUNTER_PIN, HIGH);
    wait(200);
    digitalWrite(LED_COUNTER_PIN, LOW);

    pinMode(GAS_SENSOR_PIN, INPUT);
    INT_PIN = digitalPinToInterrupt(GAS_SENSOR_PIN);
    attachInterrupt(INT_PIN, onPulse, RISING);
}

void presentation()
{
    sendSketchInfo("Gas meter sensor", "0.4");
    present(CHILD_ID_PULSE_COUNTER, S_POWER, "Pulse count");
    present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Battery voltage");
}

void loop()
{
 #ifdef MY_DEBUG
  loop50ms();
 #endif
  loop15s();
  loop2m();
}

#ifdef MY_DEBUG
unsigned long timer50ms = 0ul;
void loop50ms()
{
  if (millis() - timer50ms > 50) {
    timer50ms = millis();
    if (digitalRead(LED_COUNTER_PIN) == HIGH) {
      digitalWrite(LED_COUNTER_PIN, LOW);
    }
  }
}
#endif

unsigned long timer15s = 0ul;
void loop15s()
{
  if (millis() - timer15s > 15000) {
    timer15s = millis();
            
    if (countReceived == true) {
      send(msgLitrePM.set(litresPerMinute, 2));
    } else {
      request(CHILD_ID_PULSE_COUNTER, V_WATT);
    }
  }
}

volatile unsigned long timer2m = 0ul;
void loop2m()
{
  if (millis() - timer2m > 120000) {
    // No pulse count change for 2 minutes, send msg and sleep
    if (lastPC == pulseCounter) {
      litresPerMinute = 0;
      send(msgLitrePM.set(litresPerMinute, 2));
     #ifdef MY_DEBUG
      Serial.println("No pulse chage for 2 min, going to sleep");
     #endif
      if (smartSleep(INT_PIN, RISING, 300000) < 0) {
        // No event received, sleep immediately again - no timer reset
        checkBattery();
       #ifdef MY_DEBUG
        Serial.println("Wake up after 5 min");
       #endif
      } else {
        // Event received, this fisrt is not handled by interrupt routine - increase counter here
        pulseCounter++;
        lastPulseTime = millis();
        attachInterrupt(INT_PIN, onPulse, RISING);
        timer2m = millis();
       #ifdef MY_DEBUG
        Serial.println("Wake up by pin status change");
       #endif
      }
    } else {
      // Pulse counter has been changed by ISR, wait another 2 min
      send(msgPulseCount.set(pulseCounter));
      lastPC = pulseCounter;
      timer2m = millis();
    }
  }  
}

void receive(const MyMessage &message) {
  if (message.sensor == CHILD_ID_PULSE_COUNTER) {
    if (message.type == V_WATT) {
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
  unsigned long currentPulseTime = millis();
  // Litres per minute
  litresPerMinute = 60000.0 / (currentPulseTime - lastPulseTime) * 10;
  pulseCounter++;
  lastPulseTime = currentPulseTime;
  //timer2m = millis();
 #ifdef MY_DEBUG
  digitalWrite(LED_COUNTER_PIN, HIGH);
 #endif
}

uint16_t lastVolt = 0;
void checkBattery() 
{
  // get the battery Voltage
  uint16_t volt;
  byte batteryPcnt = getBatteryStatus(volt);
  if (lastVolt != volt) {
    // Power up radio after sleep
    MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);
    send(msgVolt.set(volt));
    sendBatteryLevel(batteryPcnt);
    lastVolt = volt;
    #ifdef MY_DEBUG
     Serial.print("Battery Voltage: ");
     Serial.print(volt);
     Serial.println(" V");
    
     Serial.print("Battery percent: ");
     Serial.print(batteryPcnt);
     Serial.println(" %");
    #endif
  }
}

int getBatteryStatus(uint16_t& millivolt)
{
  // Vlim = 5,177443609022556
  // Vpb (Vlim/1024) = 0,0050610396960142
  #define VMIN 2.4 // Minimum voltage to regulator
  #define VMAX 5.0 // 5.12788104 // Vmax = 5.131970260223048
   // get the battery Voltage
   int sensorValue = analogRead(A0);
   #ifdef MY_DEBUG
    Serial.print("Anolog value (V): ");
    Serial.println(sensorValue);
   #endif
   
   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075
   // Senzor #10: R1=985K, R2=269K
   // Vmax = 5,127881040892193, Vpb = 0,0050125914378223
   float Vbat  = sensorValue * 0.005057746;  // Sensor #7: 0.0050165887;  // 1M + 270K
   int batteryPcnt = static_cast<int>(((Vbat - VMIN) / (VMAX - VMIN)) * 100.);
   millivolt = (uint16_t)(Vbat * 1000.0);

   return batteryPcnt;
}

