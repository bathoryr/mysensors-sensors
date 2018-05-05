/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */
 
#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  
// Display
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
//#include <RCSwitch.h>

#define RF24_CE_PIN 7
#define RF24_CS_PIN 8
#define RF24_PA_LEVEL_GW RF24_PA_LOW

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_LED 2
#define CHILD_ID_LED_LEVEL 3
#define CHILD_ID_LED_UPDOWN 4
#define CHILD_ID_VOLTAGE 5
#define LED_PIN 4
#define RF_TRANSMITTER 6
#define HUMIDITY_SENSOR_DIGITAL_PIN 9
#define DHTTYPE DHT22   // DHT 22  (AM2302)
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

// Advanced init (change RF24 pins)
MyTransportNRF24 transport(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_HIGH);
MyHwATMega328 hw;
//MySigningNone signer;

MySensor gw(transport, hw);

// RF433 transmitter
//RCSwitch mySwitch = RCSwitch();

DHT dht(HUMIDITY_SENSOR_DIGITAL_PIN, DHTTYPE);
float lastTemp;
float lastHum;
unsigned short lastVolt;
bool metric = true;

// Display
// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS) - connected to GND
// pin 3 - LCD reset (RST) - Not used: 0
Adafruit_PCD8544 display = Adafruit_PCD8544(6, 5, 4, 0);

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);

void setup()  
{   
  gw.begin();
  dht.begin(); 
  //mySwitch.enableTransmit(RF_TRANSMITTER);
  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("TempHumidity", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_VOLTAGE, S_MULTIMETER);
//  gw.present(CHILD_ID_LED, S_LIGHT);
//  gw.present(CHILD_ID_LED_LEVEL, S_LIGHT_LEVEL);
//  gw.present(CHILD_ID_LED_UPDOWN, S_COVER);
  
  //pinMode(LED_PIN, OUTPUT);
  metric = gw.getConfig().isMetric;

  // Init display
  display.begin();
  display.setContrast(50);
  display.display(); // show splashscreen
}

void loop()      
{  
  //gw.process();
  gw.wait(1000);
    
  float temperature = dht.readTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
  }
  display.setCursor(0,0);
  display.print(temperature);
  display.println("°C");
  display.print(humidity);
  display.print("%");
  display.display();
  unsigned short volt = readVcc();
  if (volt != lastVolt) {
    lastVolt = volt;
    gw.send(msgVolt.set(volt));
  }
  gw.sleep(SLEEP_TIME); //sleep a bit
}
/*
void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type == V_LIGHT) {
     // Change relay state
     Serial.print("Message from sensor:");
     Serial.print(message.sensor);
     Serial.print(", state:");
     Serial.println(message.getBool());
     if (message.sensor == CHILD_ID_LED)
     {
       digitalWrite(LED_PIN, message.getBool() ? 1 : 0);
       mySwitch.send(3507201, 24);
     }
  }
  if (message.type == V_LIGHT_LEVEL)
  {
     if (message.sensor == CHILD_ID_LED_LEVEL)
     {
        int level = message.getInt();
         Serial.print("Message from sensor:");
         Serial.print(message.sensor);
         Serial.print(", level:");
         Serial.println(level);
        if (level == 25)
          mySwitch.send(3507209, 24);
        if (level == 50)
          mySwitch.send(3507208, 24);
        if (level == 100)
          mySwitch.send(3507207, 24);
     }
  }
  if (message.sensor == CHILD_ID_LED_UPDOWN)
  {
    switch (message.type)
    {
      case V_UP:
        mySwitch.send(3507205, 24);
        break;
      case V_DOWN:
        mySwitch.send(3507206, 24);
        break;
      case V_STOP:
        digitalWrite(LED_PIN, message.getBool() ? 1 : 0);
        mySwitch.send(3507201, 24);
        break;
    }
  }
}
*/
//From http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
unsigned short readVcc() {
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
  return (unsigned short) result; // Vcc in millivolts
}

