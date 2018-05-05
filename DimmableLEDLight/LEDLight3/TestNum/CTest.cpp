#include <arduino.h>
#include "CTest.h"

CTest::CTest()
{
}

CTest::~CTest()
{
}

void CTest::CheckLoop()
{
	if (millis() - timer > timeout * 1000) {
		timer = millis();
		Serial.println("Timeout expired");
	}
}
