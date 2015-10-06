#pragma once

#define rand_buff_size 100000

class Pandemic_RNG
{
public:
	Pandemic_RNG(void);
	~Pandemic_RNG(void);

	int rand_buff_idx;

	int rand_buff[rand_buff_size];
	float rand_float_buff[rand_buff_size];

	int utility_uniformInt(int lower, int inclusive_upper); //gets an integer between lower <= X <= upper
	float utility_uniformFloat();

	int utility_uniformInt_noUpdate(int lower, int inclusive_upper, int offset_from_base);
	float utility_uniformFloat_noUpdate(int offset_from_base);
};

