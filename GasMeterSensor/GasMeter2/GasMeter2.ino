/* Gas meter pulse sensor - read pulses from gas meter and send pulse count and current cinsuption in dm3 (litre)
 *  On satartup, requests initial value of counter from controller - V_WATT message type.
 *  It is battery powered, it counts pulses and sends the count every 5 minutes.
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
unsigned long lastPC = 0ul;
bool countReceived = false;

void setup()
{
  analogReference(INTERNAL);
  pinMode(GAS_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GAS_SENSOR_PIN), onPulse, RISING);
  request(CHILD_ID_PULSE_COUNTER, V_WATT);
  checkBattery();
}

void presentation()
{
  sendSketchInfo("Gas meter sensor", "2.0");
  present(CHILD_ID_PULSE_COUNTER, S_POWER, "Pulse count");
  present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Battery voltage");
}

void loop()
{
  smartSleep(300000);
  // Approximates consumption - litres per minute, one pulse = 10 dm3
  float litresPerMinute = (pulseCounter - lastPC) / 5 * 10;
  // Pulse counter has been changed by ISR, wait another 5 min
  if (countReceived) {
    send(msgLitrePM.set(litresPerMinute, 2));
    send(msgPulseCount.set(pulseCounter));
  } else {
    request(CHILD_ID_PULSE_COUNTER, V_WATT);
  }
  lastPC = pulseCounter;
  checkBattery();
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
  pulseCounter++;
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
  #define VMIN 3.5 // Minimum voltage to regulator, on 8 MHz we can go down to 2.4V
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

