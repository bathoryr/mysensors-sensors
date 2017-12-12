// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
#define MY_BAUD_RATE 9600

#include <MySensors.h>

#define CHILD_ID_VCC  1
MyMessage msgVCC(CHILD_ID_VCC, V_VOLTAGE);  
unsigned long SLEEP_TIME = 300000; // Sleep time between reads (in milliseconds)

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

void setup() {
  //power_twi_disable();
  // set pins to OUTPUT and LOW  
  for (byte i = 0; i <= A5; i++) {
    // skip radio pins
    if (i >= 9 && i <= 13)
      continue;
    pinMode (i, OUTPUT);    
    digitalWrite (i, LOW);  
  }  // end of for loop
}

void presentation()  {
  sendSketchInfo("Solar Sensor", "0.2");
  present(CHILD_ID_VCC, S_MULTIMETER);  
}

void loop() {
	//send back the values
	send(msgVCC.set(readVcc()));
	// delay until next measurement (msec)
  sleep(SLEEP_TIME);
}

