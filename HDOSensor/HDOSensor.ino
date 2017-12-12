/* Detektor pro HDO, připojený na pomocné kontakty HDO stykače na D3
 * Periodicky posílá signál o stavu HDO, při změně odešle okamžitě.
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24

#include <SPI.h>
#include <MySensors.h>

#define CHILD_ID 1
#define HDO_PIN  3     // Arduino Digital I/O pin for button/reed switch
//#define HDO_PIN_INT 1  // Interrupt for D3
volatile bool HDOState;
bool LastHDOState;

MyMessage HDOmsg(CHILD_ID, V_TRIPPED);

void setup()  
{  
  // Setup the button
  pinMode(HDO_PIN, INPUT);
  // Activate internal pull-up
  digitalWrite(HDO_PIN, HIGH);
  attachInterrupt(digitalPinToInterrupt(HDO_PIN), readHDOState, CHANGE);
}

void presentation() {
  sendSketchInfo("HDO sensor", "0.2");
  present(CHILD_ID, S_DOOR, "HDO signal");  
}

//  Check if digital input has changed and send in new value
void loop() 
{
  Loop1s();
  Loop60s();
} 

// When HDO status is chsnged, send message immediately
unsigned long timer1s = 0L;
void Loop1s()
{
  if (millis() - timer1s > 1000) {
    timer1s = millis();
    if (LastHDOState != HDOState) {
      send(HDOmsg.set(HDOState) );
      LastHDOState = HDOState;
    }
  }
}

// Every 1 minute send HDO status
unsigned long timer60s = 0L;
void Loop60s()
{
  if (millis() - timer60s > 60000) {
    timer60s = millis();
    readHDOState();
    send(HDOmsg.set(HDOState) );
    LastHDOState = HDOState;
  }
}

// ISR to change HDO status immediately
void readHDOState()
{
  HDOState = (digitalRead(HDO_PIN) == LOW);
}

