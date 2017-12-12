// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE

//#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define CHILD_ID_TEMP 0
#define CHILD_ID_VOLTAGE 1
#define CHILD_ID_BOILER 2
#define CHILD_ID_HEATING 3
#define TEMP_SENSOR_DIGITAL_PIN 7
#define RELAY_PIN 4
#define LED_PIN 8

#define TARGET_TEMP 60

const unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
const byte HEARTBEAT = 20; // Heartbeat interval 10 minutes (SLEEP_TIME cycles)
unsigned int heartbeatCounter = 0;

OneWire oneWire(TEMP_SENSOR_DIGITAL_PIN);
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

float lastTemperature;
uint16_t lastVolt;
boolean metric = true; 
int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;
byte batteryPcnt;

MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);
MyMessage msgBoiler(CHILD_ID_BOILER, V_STATUS);

void setup()  
{ 
  sensors.begin();

  analogReference(INTERNAL);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("BoilerTemp", "0.4");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_TEMP, S_TEMP, "Boiler temp");
//  present(CHILD_ID_VOLTAGE, S_MULTIMETER);
  present(CHILD_ID_BOILER, S_BINARY, "Boiler led");
  present(CHILD_ID_HEATING, S_BINARY, "Boiler heat");
}

void loop()      
{
  loop60s();
  loop15min();
}

unsigned long timer60s = 0;
void loop60s()
{
  if (millis() - timer60s > (unsigned long)60 * 1000) {
    timer60s = millis();
    sendBoilerTemp();
  }
}

unsigned long timer15min = 0;
void loop15min()
{
  if (millis() - timer15min > (unsigned long)60 * 15 * 1000) {
    timer15min = millis();
    sendHeartbeat();
  }
}

void receive(const MyMessage &message)
{
  switch (message.sensor) {
  case CHILD_ID_HEATING:
    if (message.type == V_STATUS) {
      digitalWrite(RELAY_PIN, message.getBool()?HIGH:LOW);
    }
    break;
  }
}

void sendBoilerTemp()
{
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  #ifdef MY_DEBUG
    Serial.print("Temperature: ");
    Serial.println(temperature);
  #endif
  if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
    send(msgTemp.set(temperature, 1));
    lastTemperature = temperature;
    if (temperature < TARGET_TEMP - 10) {
      //digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(LED_PIN, HIGH);
      send(msgBoiler.set(true));
    }
    if (temperature >= TARGET_TEMP) {
      //digitalWrite(RELAY_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      send(msgBoiler.set(false));
    }
  }  
}

byte checkBattery() 
{
  // get the battery Voltage
  uint16_t volt;
  byte batteryPcnt = getBatteryStatus(volt);
  if (lastVolt != volt) {
    // Power up radio after sleep
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
  return batteryPcnt;
}

/* BATTERY METER
  Internal_ref=1.1V, res=10bit=2^10-1=1023, 
  Some different 1.5V cell (I use AAs) configurations and some suggested corresponding resistor value relations:
  2AA: Vin/Vbat=470e3/(1e6+470e3), 3AA: Vin/Vbat=680e3/(2.5e6+680e3), 4AA: Vin/Vbat=1e6/(1e6+5e6), 5-6AA: Vin/Vbat=680e3/(5e6+680e3), 6-8AA: Vin/Vbat=1e6/(1e6+10e6)  
  Example with 6AAs and R1=5Mohm (=two 10Mohm in parallell) and R2=680kohm:
  Vlim = 1.1*Vbat/Vin = 9.188235 V (Set eventually to Vmax) 
                          (9.2V = 1.53V/cell which is a little low for a fresh cell but let's say it's ok to stay at 100% for the first period of time)
  Volts per bit = Vlim/1023 = 0.008982
  Vmin = 6V (Min input voltage to regulator according to datasheet. Or guess Vout+1V )   DEFINE
  Vmax = 9.188235V (Known or desired voltage of full batteries. If not, set to Vlim.)  DEFINE
  Vpercent = 100*(Vbat-Vmin)/(Vmax-Vmin)
*/
int getBatteryStatus(uint16_t& millivolt)
{
  // Vlim = 5,177443609022556
  // Vpb (Vlim/1024) = 0,0050610396960142
  #define VMIN 3.3 // Minimum voltage to regulator
  #define VMAX 10.0 // 5.12788104 // Vmax = 5.131970260223048
   // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
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
   float Vbat  = sensorValue * 0.0050125914378223;  // Sensor #7: 0.0050165887;  // 1M + 270K
   int batteryPcnt = static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.);
   millivolt = (uint16_t)(Vbat * 1000.0);

   return batteryPcnt;
}
