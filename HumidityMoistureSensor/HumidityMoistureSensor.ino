// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

// NK5110 display is connected
//#define USING_DISPLAY 

#include <MySensors.h>  
#include <dht.h>  

#ifdef USING_DISPLAY
// Display
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fonts/FreeSansBold9pt7b.h>
#endif

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_MOIST 2
#define CHILD_ID_VOLTAGE 3

#define HUMIDITY_SENSOR_PIN 8
#define HUMIDITY_SENSOR_VCC 7
#define LED_PIN 6
#define MOISTURE_SENSOR_VCC 5
#define MOISTURE_SENSOR_PIN A1

const unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)
const byte HEARTBEAT = 10; // Heartbeat interval 10 minutes (SLEEP_TIME cycles)
unsigned int heartbeatCounter = 0;

dht dht1;
float lastTemp;
int lastHum;
int lastSoil;
uint16_t lastVolt;
boolean metric = true; 
int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point
int oldBatteryPcnt = 0;
byte batteryPcnt;
//int getBatteryStatus();

// Display
// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS) - connected to GND
// pin 3 - LCD reset (RST) - to RST pin MCU
#ifdef USING_DISPLAY
Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 0);
#endif

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);
MyMessage msgSoil(CHILD_ID_MOIST, V_HUM);

void setup()  
{ 
  pinMode(HUMIDITY_SENSOR_VCC, OUTPUT);
  digitalWrite(HUMIDITY_SENSOR_VCC, LOW);
  pinMode(MOISTURE_SENSOR_VCC, OUTPUT);
  digitalWrite(MOISTURE_SENSOR_VCC, LOW);
  pinMode(LED_PIN, OUTPUT);
  for(int i=0; i<3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
  // Wait for configuration for 10 seconds, then timeout and start the sketch
  unsigned long start=millis();
  Serial.println("Enter cmd in 10s");
  while((millis() - start) < 10000) {
    if (Serial.available()) {
      handleInput(Serial.read());
      start=millis();
    }
  }
      
  #ifdef USING_DISPLAY
    // Init display
    display.begin();
    display.setContrast(50);
    //display.display(); // show splashscreen
  #endif
  metric = getConfig().isMetric;
  analogReference(INTERNAL);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity&Soil", "0.3");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_MOIST, S_HUM, "Moisture");
  present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Battery");
}

void loop()      
{  
  if (heartbeatCounter++ % HEARTBEAT == 0) {
    batteryPcnt = checkBattery();
  }
  float temperature;
  int humidity;
  if (readTemp(temperature, humidity)) {
    if (temperature != lastTemp) {
      lastTemp = temperature;
      send(msgTemp.set(temperature, 1));
    }
    if (humidity != lastHum) {
      lastHum = humidity;
      send(msgHum.set(humidity));
    }
    #ifdef USING_DISPLAY
      display.clearDisplay();
      display.setFont(&FreeSansBold9pt7b);
      display.setCursor(0, 12);
      //display.println();
      display.print(temperature, 1);
      display.print(" C");
      display.setCursor(0, 30);
      display.print(humidity, 1);
      display.print(" %");
      display.setFont(NULL);
      display.setCursor(0, 38);
      display.print("Batt:");
      display.print(batteryPcnt);
      display.print("%");
      display.display();
    #endif
  } else {
    #ifdef USING_DISPLAY
      display.clearDisplay();
      display.print("DHT error.");
    #endif
    Serial.println("Failed reading temperature from DHT");
  }
  int soil = readSoil();
  if (soil != lastSoil) {
    lastSoil = soil;
    send(msgSoil.set(soil));
  }
  sleep(SLEEP_TIME); //sleep a bit
}

bool readTemp(float& temp, int& hum)
{
  digitalWrite(HUMIDITY_SENSOR_VCC, HIGH);
  sendHeartbeat();
  wait(1100);
  bool result = dht1.read11(HUMIDITY_SENSOR_PIN) == DHTLIB_OK;
  temp = dht1.temperature;
  hum = (int)dht1.humidity;
  digitalWrite(HUMIDITY_SENSOR_VCC, LOW);
  #ifdef MY_DEBUG
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(", hum: ");
    Serial.print(hum);
    Serial.print(", status: ");
    Serial.println(result);
  #endif
  return result;
}

int readSoil()
{
  digitalWrite(MOISTURE_SENSOR_VCC, HIGH);
  delay(20);
  int value = analogRead(MOISTURE_SENSOR_PIN);
  digitalWrite(MOISTURE_SENSOR_VCC, LOW);
  #ifdef MY_DEBUG
    Serial.print("Soil: ");
    Serial.println(value);
  #endif
  return value;
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
  #define VMAX 4.7 // 5.12788104 // Vmax = 5.131970260223048
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
   float Vbat  = sensorValue * 0.0050298685;  // Sensor #7: 0.0050165887;  // 1M + 270K
   int batteryPcnt = static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.);
   millivolt = (uint16_t)(Vbat * 1000.0);
   return batteryPcnt;
}

static byte value;

static void handleInput (char c) {
    if ('0' <= c && c <= '9')
        value = 10 * value + c - '0';
    else if ('a' <= c && c <='z') {
        Serial.print("> ");
        Serial.print((int) value);
        Serial.println(c);
        switch (c) {
          case 'c': // set node id
            hwWriteConfig(EEPROM_NODE_ID_ADDRESS, 0xff);
            hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, 0xff);
            hwWriteConfig(EEPROM_DISTANCE_ADDRESS, 0xff);
            Serial.println("NodeID cleared");
            hwReboot();
          break;
        }
    }
}                             
