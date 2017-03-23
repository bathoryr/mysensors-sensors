/* Gas meter pulse sensor - read pulses from gas meter and send pulse count and current cinsuption in dm3 (litre)
 *  On satartup, requests initial value of counter from controller - V_WATT message type.
 *  It is battery powered, so it goes to sleep when no pulse received after 2 minutes. Wakes up on next pulse and sends 
 *  current consumption every 15 seconds.
 *  In debug mode flash LED every pulse detected.
 *  
 *  Mounting pulse sensor on Gas meter:
 *  Place reed switch close to last digit on the mechanical counter. There is little magnet on disc in nuber 6 or 0.
 *  
 *  CONFIG: MYSensors bootloader, internal clock 8MHz.
 */

//#define MY_DEBUG
#define MY_RADIO_NRF24
#include <MySensors.h>

#define GAS_SENSOR_PIN  2
#define LED_COUNTER_PIN 6

#define CHILD_ID_PULSE_COUNTER 1
#define CHILD_ID_VOLTAGE 2
MyMessage msgPulseCount(CHILD_ID_PULSE_COUNTER, V_WATT);
MyMessage msgLitrePM(CHILD_ID_PULSE_COUNTER, V_KWH);

volatile unsigned long pulseCounter = 0ul;
unsigned long lastPulseTime = millis();
float litresPerMinute;
unsigned long lastPC = 0ul;
byte INT_PIN;
bool countReceived = false;

void setup()
{
    analogReference(INTERNAL);
    request(CHILD_ID_PULSE_COUNTER, V_WATT);
    #ifdef MY_DEBUG
      pinMode(LED_COUNTER_PIN, OUTPUT);
      digitalWrite(LED_COUNTER_PIN, HIGH);
      wait(200);
      digitalWrite(LED_COUNTER_PIN, LOW);
    #endif
    pinMode(GAS_SENSOR_PIN, INPUT);
    INT_PIN = digitalPinToInterrupt(GAS_SENSOR_PIN);
    attachInterrupt(INT_PIN, onPulse, RISING);
    checkBattery();
}

void presentation()
{
    sendSketchInfo("Gas meter sensor", "1.1");
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
// In debug mode, flash LED every incoming pulse
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

float last_lpm = 0;
unsigned long timer15s = 0ul;
void loop15s()
{
  // changed to 30s interval
  if (millis() - timer15s > 30000) {
    timer15s = millis();
            
    if (countReceived == true) {
      if (last_lpm != litresPerMinute) {
        last_lpm = litresPerMinute;
        send(msgLitrePM.set(litresPerMinute, 2));
      }
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
  // Litres per minute, one pulse = 10 dm3
  litresPerMinute = 60000.0 / (currentPulseTime - lastPulseTime) * 10;
  pulseCounter++;
  lastPulseTime = currentPulseTime;
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
  #define VMIN 3.6 // Minimum voltage to regulator, on 8 MHz we can go down to 2.4V
  #define VMAX 5.1 // 5.12788104 
   // get the battery Voltage
   int sensorValue = analogRead(A0);
   #ifdef MY_DEBUG
    Serial.print("Anolog value (V): ");
    Serial.println(sensorValue);
   #endif
   
   // 1M, 270K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.01 uF cap to reduce noise at that point
   // ((1e6+270e3)/270e3)*1.1 = Vmax = 5.174074 Volts
   // 5.174074/1023 = Volts per bit = 0.005057746
   // Vmax = 5,127881040892193, Vpb = 0,0050125914378223
   float Vbat  = sensorValue * 0.005057746;
   int batteryPcnt = static_cast<int>(((Vbat - VMIN) / (VMAX - VMIN)) * 100.);
   millivolt = (uint16_t)(Vbat * 1000.0);

   return min(batteryPcnt, 100);
}

