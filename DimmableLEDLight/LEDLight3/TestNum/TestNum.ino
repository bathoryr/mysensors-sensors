/*
 Name:		TestNum.ino
 Created:	3/25/2018 9:38:20 PM
 Author:	batho
*/
#include "CTest.h"

CTest test;
// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
}

unsigned long timer1s = millis();

void loop() {
	if (millis() - timer1s > 1000) {
		timer1s = millis();
		test.CheckLoop();
	}
}
