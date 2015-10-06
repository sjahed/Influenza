#pragma once
#include "simParameters.h"
#include "Location.h"
#include "Person.h"
#include "LocationArray.h"
#include "DailyLocationArray.h"
#include "Pandemic_RNG.h"
#include "DoctorExperiment.h"
#include <vector>

#include <set>
#include <cstdio>
using namespace std;

#define MAIN_DELAY_SECONDS 0
#define NAME_OF_SIM_TYPE "cpu_singlethreaded"
#define NAME_OF_SIM_DEVICE "cluster"

#define CONSOLE_OUTPUT 0

typedef unsigned long randCtr_t;
typedef unsigned long randKey_t;

class PandemicSim
{
public:
	PandemicSim();
	~PandemicSim(void);


	void setup_getUniquePeople(vector<Person *> *, int);
	void setup_getPeopleFromRandomWorkplaceOfType(vector<Person *> *, int, int);

	//public methods to start and stop simulation

	void setupAndRunSim();	//sets up simulation and runs to completion
	void timeSimulation();


	//methods for simulation setup

	void setupSimulation();	//do complete setup
	void setup_initRandomBuffer(int seed);	//fill the random buffer and float buffer
	void setup_loadSimParameters();			//read simulation parameters from file
	void setup_loadCrossImmunityParameters();
	void setup_hoursThatMatter();
	void setup_scaleSimulation();			//scales the number of people and locations by an arbitrary factor
	void setup_generatePeople(DailyLocationArray * locs);	//sets up people and assigns household/workplace
	void setup_generatePeople_assignChildAgeAndSchool(Person * p);
	int setup_generatePeople_assignAdultWorkplace();
	void setup_generateBusinesses(DailyLocationArray * locs);	//sets up workplace location data
	void setup_initialInfection();	//generates initial infected population
	void setup_buildStaticLocArrays();

	//methods for debugging and validation

	void debug_openStreams();
	void debug_dumpPeople();
	void debug_dumpContacts();
	void debug_dumpLocations(LocationArray * locArray);

	//methods to generate schedules each day

	void generateSchedules();	//generate schedules and DailyLocationArray for today
	void schedule_personWeekday(int person_idx);	//generates a weekday schedule for person p
	void schedule_personWeekend(int person_idx);	//generates a weekend schedule for person p
	void schedule_threeErrands (Person * p, int low_hour, int high_hour);		//assigns three errands between hours low <= X <= high
	int  schedule_getErrandDestination();		//gets a random errand destination according to weekday/weekend PDF
	void schedule_countPersonStatus(int person_idx);


	LocationArray * makeContacts_getLocArray(int weekday, int hour);

	//methods to generate contacts for infected people

	void makeContacts_iterateHours();		//make contacts by hour
	void makeContacts_iterateHourlyQueue(int hour);	//iterates hourly queues to select infected to make contacts
	int makeContacts_Person(Person * p, int hour, Person ** outputIds, int * outputKvals);	//make contacts for person p at location l
	void makeContacts_printHourlyOutput(LocationArray * locs);	//prints pretty output

	//methods to do daily update - consume contacts and update infected status

	void dailyUpdate();	//iterate infected people, make infections, and recover if they've culminated
	void daily_makeInfections(Person * p);	//consume contacts for person p and infect victims
	void daily_recoverIfCulminated(Person * p);	//if a person has reached culmination, recover them
	double daily_calculateInfectionThreshold (Person * p, float repnum, int profile, int infection_day);

	FILE * fInfectedStatistics;
	void output_dailyInfectedStatusCount();
	void output_dailyTransmissions();

	//methods for transmitting an infection

	void transmitInfection_pandemic(Person * victim);
	void transmitInfection_seasonal(Person * victim);
	void setReproductionNumber(Person * p, int action);	//calculates reproduction according to SIR status adjusted by epsilons

	void cleanup();
	void cleanup_calculateReproduction();	//calculates final reproduction numbers
	void cleanup_calculateReproductionBySymptomatic();


//private:
	int simulation_max_days;


	void infected_watch();

	vector<Person> peopleArray;

	FILE * output_stats_file;
	FILE * sampling_data;

	FILE * fDailyInfectionCounts;

	FILE * infectedWatch;
	FILE * debug_asserts;

	float asymp_factor;// factor by which the infectiousness profile of an asymptomatic is reduced. asymp = 4.4954/20.2094.  Cite: Carrat08
	float epsilon_p;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile that is also occuring(e.g. cross-immunity created by the seasonal virus)
	float epsilon_pr;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile that has already passed(e.g. cross-immunity created by the seasonal virus)
	float epsilon_ps;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile when both are acurring at the same time(e.g. cross-immunity created by the seasonal virus)
	float epsilon_s;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile that is also occuring(e.g. cross-immunity created by the pandemic virus)
	float epsilon_sr;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile that has already passed(e.g. cross-immunity created by the pandemic virus)
	float epsilon_sp;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile when both are acurring at the same time(e.g. cross-immunity created by the pandemic virus)
	float k_hh;// proportion of duration and closeness of a household contact with respect to the household contact  k_hh=1. Cite: Mossong06
	float k_wp;// proportion of duration and closeness of a workplace contact with respect to the household contact k_wp=0.67.  Cite: Mossong06
	float k_er;// proportion of duration and closeness of an errand contact with respect to the household contact k_er=0.44. Cite: Mossong06

	float percent_symptomatic;

	float k_values[number_location_types];

	float gamma1[CULMINATION_PERIOD][2], gamma2[CULMINATION_PERIOD][2]; 
	float lognorm1[CULMINATION_PERIOD][2], lognorm2[CULMINATION_PERIOD][2];
	float weib1[CULMINATION_PERIOD][2], weib2[CULMINATION_PERIOD][2];

	int seed;
	inline int days_elapsed();

	int initial_infected_pandemic, initial_infected_seasonal;
	float reproduction_number_pandemic, reproduction_number_seasonal;

	float household_data[number_household_types][3];
	float workplace_data[number_business_types][7];
	float child_age_data[number_child_age_types][3];
	float adult_age_data[number_adult_age_types][3];

	int number_people;
	int number_workplaces;
	int number_households;
	double people_scaling_factor, location_scaling_factor;
	int number_locations;

	int workplace_type_cumulative_count[number_business_types];

	FILE * fContacts;

	void dump_contact(int person, 
		int day_p, double p_p, 
		int day_s, double p_s, 
		float k_val, double z_p, double z_s);


	void deprecated_hourlyScheduler_buildLocArray(LocationArray * currentArray, int hour);

	vector<Person *> infected_array;


	vector<Person *> infectedHourlyQueue[24];

	DailyLocationArray dailyLocArray;

	vector<int> weekday_hours_that_matter;
	vector<int> weekend_hours_that_matter;

	int reproduction_pandemic[MAX_MAX_DAYS];
	int reproduction_seasonal[MAX_MAX_DAYS];

	int count_pandemic_status[4];
	int count_seasonal_status[4];

	Pandemic_RNG * myRNG;


	//////////////////////////////////////////////////////////////////////////
	//doctor experiment

	DoctorExperiment * doctorExperiment;

};

int roundHalfUp_toInt(double d);
void count_age_and_location(int age,int loc_type);
