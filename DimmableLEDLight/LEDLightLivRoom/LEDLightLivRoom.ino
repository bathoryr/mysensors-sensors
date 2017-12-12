/* Dimmable LED light
 * Functions: 
 * Motion sensor & lignt intensity: Measure lux level and motion.
 * Has no functions for controlling light - it is provided by controller
 * When OpenHAB receive MoveDetectMsg - it should set/delete timer to turn light off
 */

// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <MySensors.h>  
#include <BH1750.h>
#include <Wire.h> 

#define LIGHT_OFF false
#define LIGHT_ON true

#define LED_PIN 3       // Test LED - not used
#define OUTPUT_PIN 5    
#define MOTION_LED 7    // Move indication LED
// The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt - we needn't it)
#define DIGITAL_INPUT_SENSOR 4   

bool LastLightState = LIGHT_OFF;
byte LastDimValue;
bool EnableMotionDetect = true;           // Motion sersor activation
uint16_t LastLightLevel;

#define CHILD_ID_LIGHT 1
#define CHILD_ID_MOTION 2
#define CHILD_ID_LUX 3
#define CHILD_ID_MOVE_DETECT 5

MyMessage motionMsg(CHILD_ID_MOTION, V_TRIPPED);
MyMessage lightMsg(CHILD_ID_LUX, V_LEVEL);

// Light sensor
BH1750 lightSensor;

void setup()  
{ 
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(MOTION_LED, OUTPUT);
  pinMode(DIGITAL_INPUT_SENSOR, INPUT);     // Set the motion sensor digital pin as input
  lightSensor.begin();  

  request(CHILD_ID_LIGHT, V_DIMMER);
  wait(100);
  request(CHILD_ID_LIGHT, V_STATUS);
  request(CHILD_ID_MOVE_DETECT, V_STATUS);
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Dimmable Light", "0.3");

  present(CHILD_ID_LIGHT, S_DIMMER, "Light intensity");
  present(CHILD_ID_MOTION, S_MOTION, "Motion activity");
  present(CHILD_ID_LUX, S_LIGHT_LEVEL, "Illumination level");
  present(CHILD_ID_MOVE_DETECT, S_BINARY, "Move detect act");
}

void loop()      
{
  Loop1s();
  Loop5s();
  Loop60s();
}

// Every one second - detect and send motion info
// Light on motion detection is controlled by OpenHAB
boolean LastTripped = false;
unsigned long timer1s = 0;
void Loop1s()
{
  if (millis() - timer1s > 1000) {
    timer1s = millis();
    // Send motion sensor info
    if (EnableMotionDetect) {
      boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH;
      if (tripped != LastTripped) {
        LastTripped = tripped;
        digitalWrite(MOTION_LED, tripped);
        send(motionMsg.set(tripped));
      }
    }
  }
}

// measure light intensity
unsigned long timer5s = 0;
void Loop5s()
{
  if (millis() - timer5s > 5000) {
    timer5s = millis();
    // Get Lux value
    SendLightLevel();
  }
}

unsigned long timer60s = 0;
void Loop60s()
{
  if (millis() - timer60s > 60000) {
    timer60s = millis();
    // Every minute send light level always
    LastLightLevel = 999;
    SendLightLevel();
  }
}

void SendLightLevel() 
{
  uint16_t lux = lightSensor.readLightLevel();
  // Tolerance for light level
  unsigned int tolerance = lux / 10;
  if (lux < max(0, LastLightLevel - tolerance) || lux > LastLightLevel + tolerance) {
    LastLightLevel = lux;
    send(lightMsg.set(lux));
   #ifdef MY_DEBUG
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
   #endif
  }
}

void receive(const MyMessage &message)
{
  bool lstate = LastLightState;
  byte dimvalue = LastDimValue;
  switch (message.sensor) {
    case CHILD_ID_LIGHT:
      if (message.type == V_STATUS) {
        // Message from light switch (ON/OFF)
        lstate = message.getBool();
       #ifdef MY_DEBUG
        Serial.println( "V_LIGHT command received..." );
        Serial.print("Setting value: ");
        Serial.println(lstate);
       #endif
      }
      if (message.type == V_DIMMER) {
        // Message from dimmer (intensity 0 - 100)
        dimvalue = message.getInt();
       #ifdef MY_DEBUG
        Serial.println( "V_DIMMER command received..." );  
        Serial.print("Setting value: ");
        Serial.println(dimvalue);
       #endif
      }
      SetCurrentState2Hardware(lstate, dimvalue);
      break;
    case CHILD_ID_MOVE_DETECT:
      if (message.type == V_STATUS) {
        // Command from OpenHAB - Enable/disable move detection 
        EnableMotionDetect = message.getBool();
        if (EnableMotionDetect == false) {
          // When message is received and the motion is active, deactivate motion indicator
          send(motionMsg.set(false));
          LastTripped = false;
          digitalWrite(MOTION_LED, false);
        }
      }
      break;
  }
}

void SetCurrentState2Hardware(bool lightState, byte dimValue)
{
  if (lightState != LastLightState) {
    if (lightState == false) {
       digitalWrite(LED_PIN, LOW);
       digitalWrite(OUTPUT_PIN, LOW);
    } else {
       int val = map(dimValue, 0, 100, 0, 255);
       analogWrite(LED_PIN, val);
       analogWrite(OUTPUT_PIN, val);
       #ifdef MY_DEBUG
       Serial.print(" - mapped to: ");
       Serial.println(val);
       #endif
    }
    LastLightState = lightState;
  }
  if (dimValue != LastDimValue) {
    if (lightState == LIGHT_ON) {
       int val = map(dimValue, 0, 100, 0, 255);
       analogWrite(LED_PIN, val);
       analogWrite(OUTPUT_PIN, val);
       #ifdef MY_DEBUG
       Serial.print(" - mapped to: ");
       Serial.println(val);
       #endif      
    }
    LastDimValue = dimValue;
  }
}

