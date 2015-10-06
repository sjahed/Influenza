#include "Person.h"
//#include "omp_types.h"
#include "simParameters.h"

using namespace std;

Person::Person(void)
{
	//virus data	
	status_pandemic = STATUS_UNEXPOSED;
	status_seasonal = STATUS_UNEXPOSED;
	generation_pandemic = -1;
	generation_seasonal = -1;
	day_pandemic_infection = -1;
	day_seasonal_infection = -1;
	profile_pandemic = -1;
	profile_seasonal = -1;
	reproduction_initial_pandemic = -1;
	reproduction_initial_seasonal = -1;

	contacts_made = 0;
	cumulative_k = 0.0;

	//scheduling data
	unemployed = false;

	doctor_day = -1;
}


Person::~Person(void)
{
}
#define weekend() (current_day == SATURDAY || current_day == SUNDAY? 1 : 0)

bool Person::is_infected(Person * p)
{
	return infected_pandemic(p) || infected_seasonal(p);
}

bool Person::infected_pandemic(Person * p)
{
	return p->status_pandemic == STATUS_ASYMPTOMATIC || p->status_pandemic == STATUS_SYMPTOMATIC;
}

bool Person::infected_seasonal(Person * p)
{
	return p->status_seasonal == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_ASYMPTOMATIC;
}

bool Person::infected_both(Person * p)
{
	return infected_pandemic (p) && infected_seasonal(p);
}