/* Dimmable LED light
 * Functions: 
 * Motion sensor & lignt intensity: Turn on light when motion detected and Lux less than limit
 * Remote control: Turn on light on by green button, when light is on, then second press turns off timeout
 *    Turn off by red button, when light is off turn off motion detection
 * When OpenHAB receive MoveDetectMsg - it should set/delete timer to turn light off
 * Config: Arduino Nano + MYS bootloader
 */

// Enable debug prints
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_REPEATER_FEATURE
const unsigned long RC_ID = 11763000;    // RC2 - Pergola

#include <MySensors.h>  
#include <RCSwitch.h>
#include <BH1750.h>
#include <Wire.h> 
#include <DallasTemperature.h>
#include <OneWire.h>

OneWire oneWire(8); // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature TempSensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
// arrays to hold device address
DeviceAddress TempDeviceAddress;
// RF 433 receiver
RCSwitch mySwitch = RCSwitch();

// Define RC button values
enum RC_buttons {RC_PWR_OFF = 201, RC_PWR_ON = 204, RC_BRIGHT_UP = 205, RC_BRIGHT_DN = 206, 
                 RC_PCNT100 = 207, RC_PCNT50 = 208, RC_PCNT25 = 209, 
                 RC_MODE_UP = 211, RC_MODE_DN = 217, RC_SPEED_UP = 215, RC_SPEED_DN = 213};
/** RCSwitch data values for all buttons
unsigned long RC1_codes[] = {3507201, 3507204, 3507205, 3507206, 3507207, 3507208, 3507209,
                             3507211, 3507217, 3507215, 3507213};
unsigned long RC2_codes[] = {11763201, 11763204, 11763205, 11763206, 11763207, 11763208, 11763209,
                             11763211, 11763217, 11763215, 11763213};
*/

#define LIGHT_OFF false
#define LIGHT_ON true

#define LED_PIN 3       // Test LED - not used
#define OUTPUT_PIN 5    
#define MOTION_LED 7    // Move indication LED
// The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt - we need'nt it)
#define DIGITAL_INPUT_SENSOR 4   

bool LastLightState = LIGHT_OFF;
byte LastDimValue;
bool EnableMotionDetect = true;           // Motion sersor activation
uint16_t LastLightLevel;

#define CHILD_ID_LIGHT 1
#define CHILD_ID_MOTION 2
#define CHILD_ID_LUX 3
//#define CHILD_ID_MOTION_TIMEOUT 4
#define CHILD_ID_MOVE_DETECT 5
#define CHILD_ID_TEMPERATUTE 6

MyMessage switchMsg(CHILD_ID_LIGHT, V_STATUS);
MyMessage moveDetectMsg(CHILD_ID_MOVE_DETECT, V_STATUS);
MyMessage motionMsg(CHILD_ID_MOTION, V_TRIPPED);
MyMessage lightMsg(CHILD_ID_LUX, V_LEVEL);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);

// Light sensor
BH1750 lightSensor;

void getRCSwitch();
bool SendCurrentState2Controller(bool lightState, byte dimValue);
void RedLEDFlash(byte counter = 2);

void setup()  
{ 
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(MOTION_LED, OUTPUT);
  pinMode(DIGITAL_INPUT_SENSOR, INPUT);     // Set the motion sensor digital pin as input
  lightSensor.begin();
  mySwitch.enableReceive(0);                // Receiver on inerrupt 0 => that is pin #2

  // Startup up the OneWire library
  TempSensors.begin();
  TempSensors.getAddress(TempDeviceAddress, 0);
  TempSensors.setResolution(TempDeviceAddress, 11);
  // Flash LED for startup indication
  for (int i = 0; i < 3; i++) {
    digitalWrite(MOTION_LED, HIGH);
    wait(100);
    digitalWrite(MOTION_LED, LOW);
    wait(200);
  } 
  request(CHILD_ID_LIGHT, V_STATUS);
  request(CHILD_ID_LIGHT, V_DIMMER);
  request(CHILD_ID_MOVE_DETECT, V_STATUS);
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Dimmable Light", "0.9");

  present(CHILD_ID_LIGHT, S_DIMMER, "Light intensity");
  present(CHILD_ID_MOTION, S_MOTION, "Motion activity");
  present(CHILD_ID_LUX, S_LIGHT_LEVEL, "Illumination level");
  present(CHILD_ID_MOVE_DETECT, S_BINARY, "Move detect act");
  present(CHILD_ID_TEMPERATUTE, S_TEMP, "Temperature");
}

void loop()      
{
  GetRCSwitch();
  Loop1s();
  Loop5s();
  Loop60s();
}

// Every one second - measure light intensity, detect and send motion info
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
    SendTemperature1();
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

float LastTemp;
void SendTemperature1() 
{
  MyMessage tempMsg(CHILD_ID_TEMPERATUTE, V_TEMP);
  if(TempSensors.requestTemperaturesByAddress(TempDeviceAddress)) { // Send the command to get temperatures
    float tempC = TempSensors.getTempC(TempDeviceAddress);
    if (tempC != LastTemp) {
      LastTemp = tempC;
      send(tempMsg.set(tempC, 1));
    }
  } else {
    send(tempMsg.set(99.9, 1));
  }
}

void SendDimValue(int dim)
{
  if (dim != LastDimValue) {
    if (LastLightState == LIGHT_ON) {
      analogWrite(LED_PIN, dim);
      analogWrite(OUTPUT_PIN, dim);
    }
    if (dim == 0) {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(OUTPUT_PIN, LOW);
      LastLightState = LIGHT_OFF;
      send(switchMsg.set(false));
    }
    //MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);
    send(dimmerMsg.set(dim));
    LastDimValue = dim;
  }
}
void GetRCSwitch()
{
  if (mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    if (value != 0) {
      bool lightState = LastLightState;
      byte dimValue = LastDimValue;
      switch (value) {
        case RC_ID + RC_PWR_OFF:
          if (LastLightState == LIGHT_ON) {
            // First button press when light is on
            LastLightState = LIGHT_OFF;
            EnableMotionDetect = true;
            digitalWrite(LED_PIN, LOW);
            digitalWrite(OUTPUT_PIN, LOW);
            send(switchMsg.set(false));   
          } else {
            // Second button press when light is off - notify user
            EnableMotionDetect = false;
            RedLEDFlash(2);
          }
          send(moveDetectMsg.set(EnableMotionDetect));
          break;
        case RC_ID + RC_PWR_ON:
          if (LastLightState == LIGHT_OFF) {
            // First button press when light is off - OpenHAB should set timer to turn light off
            LastLightState = LIGHT_ON;
            EnableMotionDetect = true;
            analogWrite(LED_PIN, LastDimValue);
            analogWrite(OUTPUT_PIN, LastDimValue);
            send(switchMsg.set(true));
          } else {
            // Second button press when light is on - notify user - OpenHAB should cancel timer
            EnableMotionDetect = false;
            RedLEDFlash(2);
          }
          send(moveDetectMsg.set(EnableMotionDetect));
          break;
        case RC_ID + RC_BRIGHT_UP:
          dimValue = min(++dimValue, 100);
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_BRIGHT_DN:
          dimValue = max(--dimValue, 1);
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT100:
          dimValue = 100;
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT50:
          dimValue = 50;
          SendDimValue(dimValue);
          break;
        case RC_ID + RC_PCNT25:
          dimValue = 25;
          SendDimValue(dimValue);
          break;
      }
    }
    mySwitch.resetAvailable();
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
      }
      if (message.type == V_DIMMER) {
        // Message from dimmer (intensity 0 - 100)
        dimvalue = message.getInt();
        if ((dimvalue < 0) || (dimvalue > 100)) {
          RedLEDFlash();
          dimvalue = 1;
        }
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
    }
    LastLightState = lightState;
  }
  if (dimValue != LastDimValue) {
    if (lightState == LIGHT_ON) {
       int val = map(dimValue, 0, 100, 0, 255);
       analogWrite(LED_PIN, val);
       analogWrite(OUTPUT_PIN, val);
    }
    LastDimValue = dimValue;
  }
}

void RedLEDFlash(byte counter)
{
  for (byte i = 0; i < counter; i++) {
    digitalWrite(MOTION_LED, HIGH);
    delay(20);
    digitalWrite(MOTION_LED, LOW);
    delay(100);
  }
}

