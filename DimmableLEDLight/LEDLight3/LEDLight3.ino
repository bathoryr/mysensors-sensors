//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RF24
#define MY_RF24_PA_LEVEL (RF24_PA_MAX)
#include <MySensors.h>

#define OUTPUT_PIN 5
#define MOTION_LED 7 // Move indication LED
#define MOTION_PIN 4

#define CHILD_ID_LIGHT 1
#define CHILD_ID_MOTION 2
#define CHILD_ID_LUX 3
#define CHILD_ID_TIMEOUT 4
#define CHILD_ID_MOVE_DETECT 5

#include "Light.h"

unsigned long timer1s = 0;
unsigned long timer1m = 0;
Light light;

void setup()
{
    light.Setup();
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(MOTION_LED, OUTPUT);
    pinMode(MOTION_PIN, INPUT);

    request(CHILD_ID_LIGHT, V_DIMMER);
    request(CHILD_ID_LIGHT, V_STATUS);
    request(CHILD_ID_MOVE_DETECT, V_STATUS);
    request(CHILD_ID_TIMEOUT, V_LEVEL);
}

void presentation()
{
    sendSketchInfo("Dimmable Light", "2.9.2");

    present(CHILD_ID_LIGHT, S_DIMMER, "Light control");
    present(CHILD_ID_MOTION, S_MOTION, "Motion activity");
    present(CHILD_ID_LUX, S_LIGHT_LEVEL, "Illumination level");
    present(CHILD_ID_TIMEOUT, S_SOUND, "Light timeout");
    present(CHILD_ID_MOVE_DETECT, S_BINARY, "Move detect switch");
}

void loop()
{
    if (millis() - timer1s > 1000)
    {
        timer1s = millis();
        light.CheckMotion();
    }
    if (millis() - timer1m > 60000)
    {
        timer1m = millis();
        MyMessage luxMsg(CHILD_ID_LUX, V_LEVEL);
        send(luxMsg.set(light.GetAvgIllumination()));
    }
}

void receive(const MyMessage &message)
{
    switch (message.sensor)
    {
    case CHILD_ID_LIGHT:
        if (message.type == V_STATUS) {
            // Message from light switch (ON/OFF)
            message.getBool() == true ? light.TurnOn() : light.TurnOff();
        }
        if (message.type == V_DIMMER) {
            // Message from dimmer (intensity 0 - 100)
            light.SetIntensity(message.getInt());
        }
        break;
    case CHILD_ID_TIMEOUT:
        if (message.type == V_LEVEL) {
            light.SetTimeout(message.getInt());
        }
        break;
    case CHILD_ID_MOVE_DETECT: 
        if (message.type == V_STATUS) {
            light.SetMotionDetector(message.getBool());
        }
        break;
    }
}

