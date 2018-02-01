/* Humidity & temperature sensor (room thermostat)
   Display Temperature and humudity, target temperature, heating status.
   MYS bootloader, 8MHz internal clock
*/

// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_DISABLED_SERIAL
// NK5110 display is connected
#define USING_DISPLAY 

#include <MyConfig.h>
#include <MySensors.h>  
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#ifdef USING_DISPLAY
  // Display
  #include <Adafruit_GFX.h>
  #include <Adafruit_PCD8544.h>
  #include <Fonts/FreeSansBold9pt7b.h>
#endif

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_VOLTAGE 2
#define CHILD_ID_HEAT 3
#define CHILD_ID_TARGETTEMP 4
#define DHT_PIN 7
int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point

const unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)
const byte HEARTBEAT = 10;              // Heartbeat interval 10 minutes (SLEEP_TIME cycles)
byte heartbeatCounter = 0;
const byte REQUEST_RETRY = 5;           // Number of retries to request values from cocntroller
byte request_attempt = 0;

DHT_Unified dht(DHT_PIN, DHT22);

float lastTemp;
int lastHum;
uint16_t lastVolt;
byte oldBatteryPcnt = 0;
byte batteryPcnt;
bool heatingState = false;
float heatTargetTemp = NAN;


#ifdef USING_DISPLAY
  // Display
  // Software SPI (slower updates, more flexible pin options):
  // pin 7 - Serial clock out (SCLK)
  // pin 6 - Serial data out (DIN)
  // pin 5 - Data/Command select (D/C)
  // pin 4 - LCD chip select (CS) - connected to GND
  // pin 3 - LCD reset (RST) - to RST pin MCU
  Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 0);
#endif

void displayOnDisplay(float temp = NAN, int hum = NAN);

void setup()  
{ 
  dht.begin();
  #ifdef USING_DISPLAY
    SPI.begin();
    // Init display
    display.begin();
    display.setContrast(50);
    display.display();
  #endif

  analogReference(INTERNAL);
  requsest(CHILD_ID_HEAT, V_STATUS);
  request(CHILD_ID_TARGETTEMP, V_VAR1);
  wait(500);
}

void presentation()  
{ 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity", "1.0.0");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM, "Humidity");
  present(CHILD_ID_TEMP, S_TEMP, "Temperature");
  present(CHILD_ID_VOLTAGE, S_MULTIMETER, "Batt. voltage");
  present(CHILD_ID_HEAT, S_BINARY, "Heating status");
  present(CHILD_ID_TARGETTEMP, S_CUSTOM, "Set target temerature");
}

void loop()      
{  
  if (heartbeatCounter++ % HEARTBEAT == 0) {
    checkBattery();
  }
  
  // Fetch temperatures from DHT sensor
  sensors_event_t event;
  byte rdcount = 0;
  do {
    if (rdcount > 0)
      wait(200);
    dht.temperature().getEvent(&event);
  } while (isnan(event.temperature) && rdcount++ < 3);
  if (!isnan(event.temperature)) {
    if (event.temperature != lastTemp) {
      lastTemp = event.temperature;
      MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
      send(msgTemp.set(event.temperature, 1));
      #ifdef MY_DEBUG
        Serial.print("T: ");
        Serial.println(event.temperature);
      #endif
    }
  } else {
    #ifdef MY_DEBUG
      Serial.println("Failed reading temperature from DHT.");
    #endif
  }
  // Fetch humidity from DHT sensor
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    int humidity = (int)event.relative_humidity;
    if (humidity != lastHum) {
      lastHum = humidity;
      MyMessage msgHum(CHILD_ID_HUM, V_HUM);
      send(msgHum.set(humidity));
      #ifdef MY_DEBUG
        Serial.print("H: ");
        Serial.println(humidity);
      #endif
    }
  } else {
    #ifdef MY_DEBUG
      Serial.println("Failed reading temperature from DHT.");
    #endif
  }
  #ifdef USING_DISPLAY
    displayOnDisplay(event.temperature, event.relative_humidity);
  #endif
  if (isnan(heatTargetTemp) && request_attempt < REQUEST_RETRY) {
    request(CHILD_ID_TARGETTEMP, V_VAR1);
    wait(500);
    request_attempt++;
  }
  smartSleep(SLEEP_TIME); //sleep a bit
}

#ifdef USING_DISPLAY
void displayOnDisplay(float temp, int hum)
{
  display.clearDisplay();
  display.setFont(&FreeSansBold9pt7b);
  // First line
  display.setCursor(0, 12);
  display.print(lastTemp, 1);
  if (isnan(temp)) {
    display.print("!");
  }
  display.print(" *C");
  // Second line
  display.setCursor(0, 30);
  display.print(lastHum, 1);
  if (isnan(hum)) {
    display.print("!");
  }
  display.print(" %");
  // Heating symbol
  display.setCursor(70, 30);
  if (heatingState) {
    display.print("#");
  } else {
    display.print("_");
  }
  // Third line - target temp
  display.setFont(NULL);
  display.setCursor(0, 38);
  display.print("T:");
  if (heatTargetTemp == 0) {
    display.print("--");
  } else {
    display.print(heatTargetTemp, 1);
  }
  // Battery status
  display.setCursor(50, 38);
  display.print("B:");
  display.print(batteryPcnt);
  display.print("%");
  display.display();
}
#endif 

void receive(const MyMessage& message)
{
  if (message.sensor == CHILD_ID_HEAT) {
    if (message.type == V_STATUS) {
      heatingState = message.getBool();
      #ifdef MY_DEBUG
        Serial.print("Receive heating status: ");
        Serial.println(heatingState);
      #endif
      #ifdef USING_DISPLAY
        displayOnDisplay(lastTemp, lastHum);
      #endif
    }
  }
  if (message.sensor == CHILD_ID_TARGETTEMP) {
    if (message.type == V_VAR1) {
      heatTargetTemp = message.getFloat();
      #ifdef MY_DEBUG
        Serial.print("Receive heating status: ");
        Serial.println(heatTargetTemp);
      #endif
      #ifdef USING_DISPLAY
        displayOnDisplay(lastTemp, lastHum);
      #endif
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
  #define VMAX 4.5 // 5.12788104 // Vmax = 5.131970260223048
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
   // Return only valid percent values (0 - 100)
   int batteryPcnt = max(min(static_cast<int>(((Vbat-VMIN)/(VMAX-VMIN))*100.), 100), 0);
   millivolt = (uint16_t)(Vbat * 1000.0);

   return batteryPcnt;
}
