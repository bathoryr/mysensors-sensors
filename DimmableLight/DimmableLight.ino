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
 * Version 1.0 - Developed by Bruce Lacey and GizMoCuz (Domoticz)
 * Version 1.1 - Modified by hek to incorporate a rotary encode to adjust 
 *                                light level locally at node
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a dimmable led light node with a rotary 
 * encoder connected for adjusting light level. 
 * The encoder has a click button which turns on/off the light (and remembers last dim-level) 
 * The sketch fades the light (non-blocking) to the desired level. 
 *
 * Default MOSFET pin is 5
 * 
 *  Arduino      Encoder module
 *  ---------------------------
 *  5V           5V (+)  
 *  GND          GND (-)
 *  6            CLK (or putput 1)
 *  7            DT  (or output 1) 
 *  8            SW (Switch/Click)  
 */


// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_REPEATER_FEATURE

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>
#include <Encoder.h>

#define LED_PIN 5           // Arduino pin attached to MOSFET Gate pin
#define KNOB_ENC_PIN_1 6    // Rotary encoder input pin 1
#define KNOB_ENC_PIN_2 7    // Rotary encoder input pin 2
#define KNOB_BUTTON_PIN 8   // Rotary encoder button pin 
#define FADE_DELAY 5       // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
#define SEND_THROTTLE_DELAY 400 // Number of milliseconds before sending after user stops turning knob
#define SN "DimmableLED /w button"
#define SV "1.2"

#define CHILD_ID_LIGHT 1

#define EEPROM_DIM_LEVEL_LAST 1
#define EEPROM_DIM_LEVEL_SAVE 2

#define LIGHT_OFF 0
#define LIGHT_ON 1

int dimValue;
int fadeTo;
int fadeDelta;
byte oldButtonVal;
bool changedByKnob=false;
bool sendDimValue=false;
unsigned long lastFadeStep;
unsigned long sendDimTimeout;
//char convBuffer[10];
bool lightState = LIGHT_OFF;

MyMessage dimmerMsg(CHILD_ID_LIGHT, V_PERCENTAGE);
MyMessage switchMsg(CHILD_ID_LIGHT, V_STATUS);
Encoder knob(KNOB_ENC_PIN_1, KNOB_ENC_PIN_2);  
Bounce debouncer = Bounce(); 

void setup()  
{ 
  // Set knob button pin as input (with debounce)
  pinMode(KNOB_BUTTON_PIN, INPUT);
  digitalWrite(KNOB_BUTTON_PIN, HIGH);
  debouncer.attach(KNOB_BUTTON_PIN);
  debouncer.interval(5);
  oldButtonVal = debouncer.read();

  // Set analog led pin to off
  analogWrite( LED_PIN, 0);

  // Retreive our last dim levels from the eprom
  fadeTo = dimValue = 0;
  int oldLevel = loadLevelState(EEPROM_DIM_LEVEL_LAST);
  Serial.print("Sending in last known light level to controller: ");
  Serial.println(oldLevel);  
  send(dimmerMsg.set(oldLevel), true);   

  Serial.println("Ready to receive messages...");  
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  present(CHILD_ID_LIGHT, S_DIMMER);
  sendSketchInfo(SN, SV);
}

void loop()      
{
  // Check if someone turned the rotary encode
  checkRotaryEncoder();

  // Check if someone has pressed the knob button
  checkButtonClick();

  // Fade light to new dim value
  fadeStep();
}

void receive(const MyMessage &message)
{
  if (message.sensor == CHILD_ID_LIGHT) {
    switch (message.type) {
      case V_STATUS:
        // Incoming on/off command sent from controller ("1" or "0")
        lightState = message.getBool();
        //send(switchMsg.set(lightState), true);
        Serial.print("New light state received: ");
        Serial.println(lightState);
        // Start fading to new light level
        dimValue = lightState ? 0 : loadLevelState(EEPROM_DIM_LEVEL_LAST);
        fadeTo = lightState ? loadLevelState(EEPROM_DIM_LEVEL_LAST) : 0;
        // Cancel send if user turns knob while message comes in
        changedByKnob = false;
        sendDimValue = false;
        Serial.print("Fade from: ");
        Serial.print(dimValue);
        Serial.print(" to: ");
        Serial.println(fadeTo);
        startFade();
        break;
      case V_PERCENTAGE:
        // Incoming dim-level command sent from controller (or ack message)
        fadeTo = message.getInt();
        Serial.print("New light level received: ");
        Serial.println(fadeTo);
        // Save received dim value to eeprom (unless turned off). Will be
        // retreived when a on command comes in
        if (fadeTo != 0)
          saveLevelState(EEPROM_DIM_LEVEL_SAVE, fadeTo);
        if (!changedByKnob) 
          knob.write(fadeTo); 
      
        saveLevelState(EEPROM_DIM_LEVEL_LAST, fadeTo);
        // Cancel send if user turns knob while message comes in
        changedByKnob = false;
        sendDimValue = false;
      
        // Start fading to new light level
        startFade();
        break;
      default:
        Serial.print("Unknown message type received: ");
        Serial.println(message.type);
        break;
    }
  }
}

void checkRotaryEncoder() {
  long encoderValue = knob.read();

  if (encoderValue > 100) {
    encoderValue = 100;
    knob.write(100);
  } else if (encoderValue < 0) {
    encoderValue = 0;
    knob.write(0);
  }

  if (encoderValue != fadeTo) { 
    fadeTo = encoderValue;
    changedByKnob = true;
    startFade();
  }
}

void checkButtonClick() {
  debouncer.update();
  byte buttonVal = debouncer.read();
  if (buttonVal != oldButtonVal && buttonVal == LOW) {
    //lightState = !lightState;
    send(switchMsg.set((bool)!lightState), true);
  }
  oldButtonVal = buttonVal;
}

void startFade() {
  fadeDelta = ( fadeTo - dimValue ) < 0 ? -1 : 1;
  lastFadeStep = millis();
}

// This method provides a graceful none-blocking fade up/down effect
void fadeStep() {
  unsigned long currentTime  = millis();
  if ( dimValue != fadeTo && currentTime > lastFadeStep + FADE_DELAY) {
    dimValue += fadeDelta;
    analogWrite( LED_PIN, (int)(dimValue / 100. * 255) );
    lastFadeStep = currentTime;
      Serial.print("Fading: ");
      Serial.println(dimValue);

    if (fadeTo == dimValue) {
      if (changedByKnob) {
        sendDimValue = true;
        sendDimTimeout = currentTime;
      }
    }
  } 
  // Wait a few millisecs before sending in new value (if user still turns the knob)
  if (sendDimValue && currentTime > sendDimTimeout + SEND_THROTTLE_DELAY)  {
     // We're done fading.. send in new dim-value to controller.
     // Send in new dim value with ack (will be picked up in incomingMessage) 
    send(dimmerMsg.set(dimValue), true); // Send new dimmer value and request ack back
    sendDimValue = false;
  }
}

// Make sure only to store/fetch values in the range 0-100 from eeprom
int loadLevelState(byte pos) {
  return min(max(loadState(pos),0),100);
}

void saveLevelState(byte pos, byte data) {
  saveState(pos,min(max(data,0),100));
}
