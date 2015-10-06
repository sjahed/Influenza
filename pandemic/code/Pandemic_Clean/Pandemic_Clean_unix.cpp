#include "stdafx.h"
#include "PandemicSim.h"
#include <unistd.h>

void delay_start()
{
	int milliseconds = MAIN_DELAY_SECONDS * 1000000;
	int ret = usleep(milliseconds);
//	printf("usleep returned: %d\n",ret);
}


int main(int argc, char * argv[])
{
  if(MAIN_DELAY_SECONDS > 0)
    delay_start();

	PandemicSim *sim = new PandemicSim(1);

	sim->timeSimulation();
}