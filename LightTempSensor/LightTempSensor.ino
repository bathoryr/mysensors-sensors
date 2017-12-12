 /*
 * REVISION HISTORY
 * Version 1.0 - idefix
 * 
 * DESCRIPTION
 * Arduino BH1750FVI Light sensor
 * communicate using I2C Protocol
 * this library enable 2 slave device addresses
 * Main address  0x23
 * secondary address 0x5C
 * connect the sensor as follows :
 *
 *   VCC  >>> 5V
 *   Gnd  >>> Gnd
 *   ADDR >>> NC or GND  
 *   SCL  >>> A5
 *   SDA  >>> A4
 * http://www.mysensors.org/build/light
 */
// Enable debug prints
#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensor.h>  
#include <BH1750.h>
#include <Wire.h> 
#include <DallasTemperature.h>
#include <OneWire.h>

#define CHILD_ID_LIGHT 0
#define CHILD_ID_TEMP 4
BH1750 lightSensor;

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
unsigned long SLEEP_TIME = 10000; // Sleep time between reads (in milliseconds)

// V_LIGHT_LEVEL should only be used for uncalibrated light level 0-100%.
// If your controller supports the new V_LEVEL variable, use this instead for
// transmitting LUX light level.
MyMessage msgLight(CHILD_ID_LIGHT, V_LEVEL);
MyMessage msgTemp(0, V_TEMP);  
uint16_t lastlux;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;

void setup()  
{ 
  lightSensor.begin();
  // Startup up the OneWire library
  sensors.begin();
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Light Lux Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(CHILD_ID_TEMP + i, S_TEMP);
  }
}

void loop()      
{     
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.println(lux);
  if (lux != lastlux) {
      send(msgLight.set(lux));
      lastlux = lux;
  }
  // Temperature
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);
  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
      send(msgTemp.setSensor(CHILD_ID_TEMP + i).set(temperature,1));
      lastTemperature[i]=temperature;
    }
  }
  sleep(SLEEP_TIME);
}
