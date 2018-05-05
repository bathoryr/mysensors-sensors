#pragma once
class CTest
{
	unsigned long timer;
	int timeout = 10;
public:
	CTest();
	~CTest();

	void CheckLoop();
};

