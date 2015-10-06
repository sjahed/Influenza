// Pandemic_Clean.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PandemicSim.h"
#include <Windows.h>

void delay_start()
{
	int milliseconds = MAIN_DELAY_SECONDS * 1000;
	Sleep(milliseconds);
}


int _tmain(int argc, _TCHAR* argv[])
{
	if(MAIN_DELAY_SECONDS > 0)
		delay_start();

	PandemicSim *sim = new PandemicSim();
	
	sim->timeSimulation();
	getchar();
}

