#include "Pandemic_RNG.h"
#include <cstdlib>

Pandemic_RNG::Pandemic_RNG(void)
{
	rand_buff_idx = 0;

	for(int i = 0; i < rand_buff_size; i++) 
	{
		rand_buff[i] = rand();	//get random number
		rand_float_buff[i] = (float) rand_buff[i] / (float) RAND_MAX;	//store it as a float too
	}
}


Pandemic_RNG::~Pandemic_RNG(void)
{
}

int Pandemic_RNG::utility_uniformInt(int lower, int inclusive_upper)
{
	//pre-check: is this a valid idx to look up?
	if(rand_buff_idx >= rand_buff_size)
		rand_buff_idx = 0;

	//look up from buffer and increment for next
	int val = rand_buff[rand_buff_idx++];
	
	//clip to range
	val = val % (1 + inclusive_upper - lower );
	val += lower;	//shift the position of this range

	return val;
}

float Pandemic_RNG::utility_uniformFloat()
{
	//pre-check: is this a valid idx to look up?
	if(rand_buff_idx >= rand_buff_size)
		rand_buff_idx = 0;

	//look up from buffer and increment for next
	return rand_float_buff[rand_buff_idx++];
}

int Pandemic_RNG::utility_uniformInt_noUpdate(int lower, int inclusive_upper, int offset_from_base)
{
	//get the index to look up
	int newRandomIndex = (rand_buff_idx + offset_from_base) % rand_buff_size;

	//look up from buffer
	int val = rand_buff[newRandomIndex];
	
	//clip to range
	val = val % (1 + inclusive_upper - lower);
	val += lower; //shift position of range

	return val;
}

float Pandemic_RNG::utility_uniformFloat_noUpdate(int offset_from_base)
{
	//get index to look up
	int newRandomIndex = (rand_buff_idx + offset_from_base) % rand_buff_size;

	//return from buffer
	return rand_float_buff[newRandomIndex];
}
