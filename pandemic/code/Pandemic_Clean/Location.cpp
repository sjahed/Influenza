#include "Location.h"
#include "Person.h"
#include <vector>
#include "simParameters.h"
#include <omp.h>
#include <iostream>
using namespace std;

Location::Location(void)
{
	count_hourly_total = 0;
//	count_hourly_infected = 0;
//	omp_init_lock(&mutex);
}

Location::~Location(void)
{
}

void Location::insert_person(Person * p)
{
//	if(count_hourly_total < LOCATION_MAX)
//		people[count_hourly_total++] = p;
//	else
//	{
		peopleOverflow.push_back(p);
		count_hourly_total++;
	

//	}
//	if(p->status_pandemic == SYMPTOMATIC || p->status_pandemic == ASYMPTOMATIC || p->status_seasonal == SYMPTOMATIC || p->status_seasonal == ASYMPTOMATIC)
//		count_hourly_infected++;
}
Person * Location::get_person(int idx)
{
//	if(idx < LOCATION_MAX)
//		return people[idx];
//	else
//		return peopleOverflow.at(idx - LOCATION_MAX);
	return peopleOverflow[idx];
}


void Location::Empty()
{
//	count_hourly_infected = 0;
	count_hourly_total = 0;
	peopleOverflow.resize(0); //resize to zero will never shrink, so we don't need to reallocate memory
}