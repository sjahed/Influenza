#pragma once
#include <omp.h>
using namespace std;

class Person
{
#define CONTACTS_CAPACITY 10
public:
	Person(void);
	~Person(void);

	static bool is_infected(Person *);
	static bool is_not_infected(Person * p)
	{
		return !is_infected(p);
	}
	static bool infected_pandemic(Person *);
	static bool infected_seasonal(Person *);
	static bool infected_both(Person *);

	//demographic data
	int workplace;
	int household;
	int age;
	bool unemployed;

	int id;

	//schedule data
	int schedule[24]; //what location ID this person will be at
	short int schedule_type[24]; //what type of location is it
	short int contacts_desired[24];	//how many contacts will be made there

	//schedule data for absenteeism experiment
	bool symptomatic_pandemic, symptomatic_seasonal;

	//viral data
	int status_pandemic, status_seasonal;
	int day_pandemic_infection, day_seasonal_infection;
	int generation_pandemic, generation_seasonal;
//	int reproduction_number_pandemic, reproduction_number_seasonal;
	double reproduction_initial_pandemic, reproduction_initial_seasonal;
	int profile_pandemic, profile_seasonal;

	//contact data
//	int errand_contacts_remaining;
	int contacts_made;
	Person * contact_ids[CONTACTS_CAPACITY];
	int contact_type[CONTACTS_CAPACITY];
	double cumulative_k;

	//doctor sampling experiment
	float doctor_day;
	double severity_pandemic, severity_seasonal;





	//DEPRECATED
//	int get_schedule(int current_hour, int current_day);
//	int contact_rate_hh, contact_rate_wp, contact_rate_er, contact_rate_total;
	int infections_today;



//	omp_lock_t mutex;


};

