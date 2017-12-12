// Enable debug prints
#define MY_DEBUG

//#define MY_NODE_ID 5
//#define MY_PARENT_NODE_ID 0

// Enable and select radio type attached
#define MY_RADIO_NRF24
// Repeater for other sensors in garden
//#define MY_REPEATER_FEATURE
// Is temperature sensor attached ?
//#define TEMP_SENSOR_ATTACHED
// ID's of Remote control
const unsigned long RC_ID = 3507000;     // RC1
//const unsigned long RC_ID = 11763000;    // RC2

#include <SPI.h>
#include <MySensors.h>  
#include <RCSwitch.h>
#include <BH1750.h>
#include <Wire.h> 
#ifdef TEMP_SENSOR_ATTACHED
  #include <DallasTemperature.h>
  #include <OneWire.h>
  
  OneWire oneWire(8); // Setup a oneWire instance to communicate with any OneWire devices
  DallasTemperature TempSensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
  // arrays to hold device address
  DeviceAddress TempDeviceAddress;
#endif

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

#define EEPROM_LIGHT_STATE 1
#define EEPROM_DIMMER_LEVEL 2
#define EEPROM_TIMEOUT_TIME 3
#define EEPROM_MOVE_DETECT 4

#define LIGHT_OFF false
#define LIGHT_ON true

#define LED_PIN 3
#define OUTPUT_PIN 5
#define MOTION_LED 7
// Motion sensor
#define DIGITAL_INPUT_SENSOR 4   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)

bool LastLightState = LIGHT_OFF;
byte LastDimValue;
uint16_t LastLightLevel;
bool LastMotion;
bool MoveLightsON = LIGHT_OFF;
bool EnableMotion = false;           // Motion sersor sctivation
byte MoveLightTimeout;          // Auto-move light timeout in seconds

#define CHILD_ID_LIGHT 1
#define CHILD_ID_MOTION 2
#define CHILD_ID_LUX 3
#define CHILD_ID_MOTION_TIMEOUT 4
#define CHILD_ID_MOVE_DETECT 5
#define CHILD_ID_TEMPERATUTE 6

MyMessage lightMsg(CHILD_ID_LUX, V_LEVEL);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);
MyMessage switchMsg(CHILD_ID_LIGHT, V_STATUS);
MyMessage motionMsg(CHILD_ID_MOTION, V_TRIPPED);
MyMessage moveTimeoutMsg(CHILD_ID_MOTION_TIMEOUT, V_LEVEL);
MyMessage moveDetectMsg(CHILD_ID_MOVE_DETECT, V_ARMED);

// RF 433 receiver
RCSwitch mySwitch = RCSwitch();
// Light sensor
BH1750 lightSensor;

unsigned long timer1s = 0ul;
unsigned long timer30s = 0ul;

void getRCSwitch();
bool SendCurrentState2Controller(bool lightState, byte dimValue);
void RedLEDFlash(byte counter = 2);

void setup()  
{ 
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(MOTION_LED, OUTPUT);
  pinMode(DIGITAL_INPUT_SENSOR, INPUT);     // sets the motion sensor digital pin as input
  mySwitch.enableReceive(0);                // Receiver on inerrupt 0 => that is pin #2
  lightSensor.begin();

  LastDimValue = 25; //loadState(EEPROM_DIMMER_LEVEL);
  EnableMotion = loadState(EEPROM_MOVE_DETECT);
  MoveLightTimeout = loadState(EEPROM_TIMEOUT_TIME);
  if (MoveLightTimeout == 0)
    MoveLightTimeout = 30;
  #ifdef TEMP_SENSOR_ATTACHED
    // Startup up the OneWire library
    TempSensors.begin();
    if (!TempSensors.getAddress(TempDeviceAddress, 0))
      Serial.println("Could not get DS18B20 device address!");
    // set the resolution to 11 bit (Each Dallas/Maxim device is capable of 9 - 12 bit resolutions)
    TempSensors.setResolution(TempDeviceAddress, 11);
  #endif
  Serial.println("Node ready.");  
  #ifdef MY_DEBUG
    Serial.print("Light timeout: ");
    Serial.println(MoveLightTimeout);
    Serial.print("Move detection: ");
    Serial.println(EnableMotion);
  #endif
}

void presentation() {
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Dimmable Light", "0.11");

  present(CHILD_ID_LIGHT, S_DIMMER);
  present(CHILD_ID_MOTION, S_MOTION);
  present(CHILD_ID_LUX, S_LIGHT_LEVEL);
  present(CHILD_ID_MOTION_TIMEOUT, S_CUSTOM);
  present(CHILD_ID_MOVE_DETECT, S_MOTION);
  #ifdef TEMP_SENSOR_ATTACHED
    present(CHILD_ID_TEMPERATUTE, S_TEMP);
  #endif

  send(moveTimeoutMsg.set(MoveLightTimeout));
  send(moveDetectMsg.set(EnableMotion));
}

void loop()      
{
  getRCSwitch();
  lightSensorOn();
  lightSensorOff();
  #ifdef TEMP_SENSOR_ATTACHED
    getTemperature1();
  #endif
}

#ifdef TEMP_SENSOR_ATTACHED
unsigned long timer120s = 0ul;
void getTemperature1() 
{
  if (millis() - timer120s > 120000) {
    timer120s = millis();
    MyMessage msg(CHILD_ID_TEMPERATUTE, V_TEMP);
    if(TempSensors.requestTemperaturesByAddress(TempDeviceAddress)) { // Send the command to get temperatures
      float tempC = TempSensors.getTempC(TempDeviceAddress);
      send(msg.set(tempC, 1));
    } else {
      send(msg.set(99.9, 1));
    }
  }
}
#endif

void lightSensorOn()
{
  if (millis() - timer1s > 1000) {
    // Get Lux value
    uint16_t lux = lightSensor.readLightLevel();
    // Tolerance for light level
    unsigned int tolerance = lux / 10;
    if (lux < LastLightLevel - tolerance || lux > LastLightLevel + tolerance) {
      #ifdef MY_DEBUG
      Serial.print("Light: ");
      Serial.print(lux);
      Serial.println(" lx");
      #endif
      LastLightLevel = lux;
      send(lightMsg.set(lux));
    }

    boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH;
    if (EnableMotion == true && tripped != LastMotion) {
      digitalWrite(MOTION_LED, tripped);
      LastMotion = tripped;      
      send(motionMsg.set(tripped));
      if (tripped) {
        #ifdef MY_DEBUG
        Serial.println("Move detected");
        #endif
        if (MoveLightsON == LIGHT_OFF && lux < 15) {
          if (SendCurrentState2Controller(LIGHT_ON, LastDimValue)) {
            MoveLightsON = LIGHT_ON;
          }
        }
        timer30s = millis();
      }
    }
    timer1s = millis();
  }
}

void lightSensorOff()
{
  if (millis() - timer30s > (unsigned long)MoveLightTimeout * 1000) {
    // Turn lights off only when were switched on by move sensor
    if (MoveLightsON == LIGHT_ON) {
      if (SendCurrentState2Controller(LIGHT_OFF, LastDimValue)) {
        MoveLightsON = LIGHT_OFF;
      }
    }
    timer30s = millis();
  }
}

void getRCSwitch()
{
  if (mySwitch.available()) {
    unsigned long value = mySwitch.getReceivedValue();
    if (value != 0) {
      bool lightState = LastLightState;
      byte dimValue = LastDimValue;
      #ifdef MY_DEBUG
      Serial.print("Received ");
      Serial.println(mySwitch.getReceivedValue());
      #endif
      switch (value) {
        case RC_ID + RC_PWR_OFF:
          lightState = LIGHT_OFF;
          break;
        case RC_ID + RC_PWR_ON:
          lightState = LIGHT_ON;
          break;
        case RC_ID + RC_BRIGHT_UP:
          dimValue = min(++dimValue, 100);
          break;
        case RC_ID + RC_BRIGHT_DN:
          dimValue = max(--dimValue, 1);
          break;
        case RC_ID + RC_PCNT100:
          dimValue = 100;
          break;
        case RC_ID + RC_PCNT50:
          dimValue = 50;
          break;
        case RC_ID + RC_PCNT25:
          dimValue = 25;
          break;
        case RC_ID + RC_MODE_UP:       // Mode +
          // Enable Motion detection
          if (EnableMotion == false) {
            EnableMotion = true;
            saveState(EEPROM_MOVE_DETECT, 1);
            send(moveDetectMsg.set(EnableMotion));
            RedLEDFlash();
            #ifdef MY_DEBUG
            Serial.println("Move detection enabled");
            #endif
          }
          break;
        case RC_ID + RC_MODE_DN:       // Mode -
          // Disable Motion detection
          if (EnableMotion == true) {
            EnableMotion = false;
            saveState(EEPROM_MOVE_DETECT, 0);
            send(moveDetectMsg.set(EnableMotion));
            RedLEDFlash();
            #ifdef MY_DEBUG
            Serial.println("Move detection disabled");
            #endif
          }
          break;
        case RC_ID + RC_SPEED_UP:       // Speed +
            if (MoveLightTimeout == 60) {
              MoveLightTimeout = 255;
              saveState(EEPROM_TIMEOUT_TIME, MoveLightTimeout);
              RedLEDFlash(8);
            } else
            if (MoveLightTimeout == 30) {
              MoveLightTimeout = 60;
              saveState(EEPROM_TIMEOUT_TIME, MoveLightTimeout);
              RedLEDFlash(4);
            } else
            if (MoveLightTimeout == 255) {
              MoveLightTimeout = 30;
              saveState(EEPROM_TIMEOUT_TIME, MoveLightTimeout);
              RedLEDFlash(2);
            }
            send(moveTimeoutMsg.set(MoveLightTimeout));
            #ifdef MY_DEBUG
            Serial.print("Move timeout set to ");
            Serial.print(MoveLightTimeout);
            Serial.println("s");
            #endif
            break;
      }
      // Command received from remote control, send value to controller
      if (!SendCurrentState2Controller(lightState, dimValue)) {
        RedLEDFlash(10);
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
        bool lstate = message.getBool();
        #ifdef MY_DEBUG
        Serial.println( "V_LIGHT command received..." );
        Serial.print("Setting value: ");
        Serial.println(lstate);
        #endif
        SetCurrentState2Hardware(lstate, dimvalue);
      }
      if (message.type == V_DIMMER) {
        dimvalue = message.getInt();
        if ((dimvalue < 0) || (dimvalue > 100)) {
          #ifdef MY_DEBUG
          Serial.println( "V_DIMMER data invalid (should be 0..100)" );
          #endif
          RedLEDFlash();
          dimvalue = 1;
        }
        #ifdef MY_DEBUG
        Serial.println( "V_DIMMER command received..." );  
        Serial.print("Setting value: ");
        Serial.println(dimvalue);
        #endif
        SetCurrentState2Hardware(lstate, dimvalue);
      }
      break;
    case CHILD_ID_MOTION:
      break;
    case CHILD_ID_LUX:
      break;
    case CHILD_ID_MOTION_TIMEOUT:
      break;
    case CHILD_ID_MOVE_DETECT:
      break;
    case CHILD_ID_TEMPERATUTE:
      break;
  }
}

void SetCurrentState2Hardware(bool lightState, byte dimValue)
{
  #ifdef MY_DEBUG
  Serial.print("Light state: ");
  Serial.print(lightState);
  Serial.print(", light Level: ");
  Serial.println(dimValue);
  #endif
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
    //saveState(EEPROM_DIMMER_LEVEL, dimValue);
    //Serial.print("Dimmer level saved to EEPROM");
  }
}

// When message is received from RC, send info to controller
// Pokud je ACK == true, pak gateway pošle zprávu zpět, takže ji přijme metoda receive()
bool SendCurrentState2Controller(bool lightState, byte dimValue)
{
  bool result = true;
  if (dimValue != LastDimValue) {
    // Pozor na typ dat zprávy - metoda getInt() neumí načíst byte. Převést to na stejný typ jaký přijde z UI
    result = result && send(dimmerMsg.set((int)dimValue), true);
  }
  if (dimValue == 0) {
    lightState = LIGHT_OFF;
  }

  if (lightState != LastLightState) {
    result = send(switchMsg.set(lightState), true) && result;
  }
  return result;
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


