// Enable debug prints
//#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_RF24_CE_PIN 7
#define MY_RF24_CS_PIN 8

#include <SPI.h>
#include <MySensor.h>  
#include <EmonLib.h>                 // Include Emon Library
#include <Wire.h>
#include <OneWire.h>
#include <Adafruit_BMP085.h>
#include <DallasTemperature.h>

#define CHILD_ID_TEMPIN 0
#define CHILD_ID_TEMPOUT 1
#define CHILD_ID_POWER 2
#define CHILD_ID_BATTERY 3
#define CHILD_ID_BARO 4
#define CHILD_ID_PUMPTEMP 5

#define SLEEP_INTERVAL 30000

// Dallas Temp. sensor
#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

const byte T1_INPUT = 2;             // Snímače teploty
const byte T2_INPUT = 3;
uint16_t lastIrms;
int lastTempIn;
int lastTempOut;
byte countVolt;
int lastPressure;
float lastTemp = -1;
float lastPumpTemp;

const float ALTITUDE = 410; // <-- adapt this value to your own location's altitude.
const char *weather[] = { "stable", "sunny", "cloudy", "unstable", "thunderstorm", "unknown" };
enum FORECAST
{
  STABLE = 0,     // "Stable Weather Pattern"
  SUNNY = 1,      // "Slowly rising Good Weather", "Clear/Sunny "
  CLOUDY = 2,     // "Slowly falling L-Pressure ", "Cloudy/Rain "
  UNSTABLE = 3,   // "Quickly rising H-Press",     "Not Stable"
  THUNDERSTORM = 4, // "Quickly falling L-Press",    "Thunderstorm"
  UNKNOWN = 5     // "Unknown (More Time needed)
};

const int LAST_SAMPLES_COUNT = 5;
float lastPressureSamples[LAST_SAMPLES_COUNT];

// this CONVERSION_FACTOR is used to convert from Pa to kPa in forecast algorithm
// get kPa/h be dividing hPa by 10 
#define CONVERSION_FACTOR (1.0/10.0)

int minuteCount = 0;
bool firstRound = true;
// average value is used in forecast algorithm.
float pressureAvg;
// average after 2 hours is used as reference value for the next iteration.
float pressureAvg2;

float dP_dt;
Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor 

MyMessage msgTempIn(CHILD_ID_TEMPIN, V_TEMP);
MyMessage msgTempOut(CHILD_ID_TEMPOUT, V_TEMP);
MyMessage msgPower(CHILD_ID_POWER, V_WATT);
MyMessage msgBattery(CHILD_ID_BATTERY, V_VOLTAGE);
MyMessage msgTemp(CHILD_ID_BARO, V_TEMP);
MyMessage msgBaro(CHILD_ID_BARO, V_PRESSURE);
MyMessage msgForecast(CHILD_ID_BARO, V_FORECAST);
MyMessage msgPumpTemp(CHILD_ID_PUMPTEMP, V_TEMP);

EnergyMonitor emon1;                   // Create an instance

void setup() {
  // put your setup code here, to run once:
  // TA12-100 má poměr 1000:1 a burden resistor 200ohm, kalibrace = 5 
  emon1.current(1, 5);             // Current: input pin, calibration.
  bmp.begin();
  tempSensor.begin();
}

void presentation()
{
  sendSketchInfo("Heating power Sensor", "0.5");
  present(CHILD_ID_TEMPIN, S_TEMP, "Temp input");
  present(CHILD_ID_TEMPOUT, S_TEMP, "Temp output");
  present(CHILD_ID_POWER, S_POWER, "Power consumption");
  present(CHILD_ID_BATTERY, S_MULTIMETER, "Battery voltage");
  present(CHILD_ID_BARO, S_BARO, "Baro pressure");
  present(CHILD_ID_BARO, S_TEMP, "Temperature");
}

void loop() {
  // put your main code here, to run repeatedly:
  uint16_t Irms = (uint16_t) (emon1.calcIrms(1480) * 230.);  // Calculate Irms only
  if (Irms != lastIrms) {
    send(msgPower.set(Irms));
    lastIrms = Irms;
  }
  float tempIn = GetTemperature(T1_INPUT);
  if ((int)tempIn != lastTempIn) {
    send(msgTempIn.set(tempIn, 1));
    lastTempIn = (int)tempIn;
  }
  float tempOut = GetTemperature(T2_INPUT);
  if ((int)tempOut != lastTempOut) {
    send(msgTempOut.set(tempOut, 1));
    lastTempOut = (int)tempOut;
  }
  unsigned int volt = readVcc();
  if (countVolt++ % 10 == 0) {
    send(msgBattery.set(volt));
  }
  // BMP-085 sensor
  int pressure = (int)(bmp.readSealevelPressure(ALTITUDE) / 100.0);
  float temperature = bmp.readTemperature();
  int forecast = sample(pressure);

  #ifdef MY_DEBUG
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print("Forecast = ");
  Serial.println(weather[forecast]);
  #endif

  if (temperature != lastTemp) 
  {
    send(msgTemp.set(temperature, 1));
    lastTemp = temperature;
  }

  if (pressure != lastPressure) 
  {
    send(msgBaro.set(pressure));
    send(msgForecast.set(weather[forecast]));
    lastPressure = pressure;
  }

  tempSensor.requestTemperatures();
  float pumpTemp = tempSensor.getTempCByIndex(0);
  if (lastPumpTemp != pumpTemp) {
    lastPumpTemp = pumpTemp;
    send(msgPumpTemp.set(lastPumpTemp, 1));
  }
  sleep(SLEEP_INTERVAL);
}

float GetTemperature(byte port)
{
  if (port == 0)
    return 0.0;
  // temp. for nominal resistance (almost always 25 C) 
  const int TEMPERATURENOMINAL = 25;
  // resistance at TEMPERATURENOMINAL (above)
  const int THERMISTORNOMINAL = 10000;
  // The beta coefficient of the thermistor (usually 3000-4000) 
  const int BCOEFFICIENT = 3960;
  // the value of the 'other' resistor (measure to make sure)
  const int SERIESRESISTOR = 10000;
  // how many Kelvin 0 degrees Celsius is
  const float KELVIN = 273.15;

  // convert the value to resistance 
  float average = (float)1023 / analogRead(port) - 1;
  average = SERIESRESISTOR / average;

  float steinhart = average / THERMISTORNOMINAL;
  steinhart = log(steinhart);
  steinhart /= BCOEFFICIENT;
  steinhart += 1.0 / (TEMPERATURENOMINAL + KELVIN);
  steinhart = 1.0 / steinhart;
  steinhart -= KELVIN;   // back to celsius
  return steinhart;
}  // end of getTemperature

//From http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
unsigned int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return (unsigned int) result; // Vcc in millivolts
}

float getLastPressureSamplesAverage()
{
  float lastPressureSamplesAverage = 0;
  for (int i = 0; i < LAST_SAMPLES_COUNT; i++)
  {
    lastPressureSamplesAverage += lastPressureSamples[i];
  }
  lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

  return lastPressureSamplesAverage;
}



// Algorithm found here
// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
// Pressure in hPa -->  forecast done by calculating kPa/h
int sample(float pressure)
{
  // Calculate the average of the last n minutes.
  int index = minuteCount % LAST_SAMPLES_COUNT;
  lastPressureSamples[index] = pressure;

  minuteCount++;
  if (minuteCount > 185)
  {
    minuteCount = 6;
  }

  if (minuteCount == 5)
  {
    pressureAvg = getLastPressureSamplesAverage();
  }
  else if (minuteCount == 35)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change * 2; // note this is for t = 0.5hour
    }
    else
    {
      dP_dt = change / 1.5; // divide by 1.5 as this is the difference in time from 0 value.
    }
  }
  else if (minuteCount == 65)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) //first time initial 3 hour
    {
      dP_dt = change; //note this is for t = 1 hour
    }
    else
    {
      dP_dt = change / 2; //divide by 2 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 95)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 1.5; // note this is for t = 1.5 hour
    }
    else
    {
      dP_dt = change / 2.5; // divide by 2.5 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 125)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    pressureAvg2 = lastPressureAvg; // store for later use.
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 2; // note this is for t = 2 hour
    }
    else
    {
      dP_dt = change / 3; // divide by 3 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 155)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 2.5; // note this is for t = 2.5 hour
    }
    else
    {
      dP_dt = change / 3.5; // divide by 3.5 as this is the difference in time from 0 value
    }
  }
  else if (minuteCount == 185)
  {
    float lastPressureAvg = getLastPressureSamplesAverage();
    float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
    if (firstRound) // first time initial 3 hour
    {
      dP_dt = change / 3; // note this is for t = 3 hour
    }
    else
    {
      dP_dt = change / 4; // divide by 4 as this is the difference in time from 0 value
    }
    pressureAvg = pressureAvg2; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
    firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
  }

  int forecast = UNKNOWN;
  if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
  {
    forecast = UNKNOWN;
  }
  else if (dP_dt < (-0.25))
  {
    forecast = THUNDERSTORM;
  }
  else if (dP_dt > 0.25)
  {
    forecast = UNSTABLE;
  }
  else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
  {
    forecast = CLOUDY;
  }
  else if ((dP_dt > 0.05) && (dP_dt < 0.25))
  {
    forecast = SUNNY;
  }
  else if ((dP_dt >(-0.05)) && (dP_dt < 0.05))
  {
    forecast = STABLE;
  }
  else
  {
    forecast = UNKNOWN;
  }

  // uncomment when debugging
  //Serial.print(F("Forecast at minute "));
  //Serial.print(minuteCount);
  //Serial.print(F(" dP/dt = "));
  //Serial.print(dP_dt);
  //Serial.print(F("kPa/h --> "));
  //Serial.println(weather[forecast]);

  return forecast;
}
