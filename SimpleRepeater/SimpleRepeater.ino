#define MY_DEBUG
#define MY_RADIO_NRF24
#define MY_PARENT_NODE_ID 0
#define MY_REPEATER_FEATURE
#define MY_RX_MESSAGE_BUFFER_FEATURE
#define MY_RF24_IRQ_PIN 2
#include <MyConfig.h>
#include <MySensors.h>  

void presentation() {
  sendSketchInfo("Simple repeater", "1.0");
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
