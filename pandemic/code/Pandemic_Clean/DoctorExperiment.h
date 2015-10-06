#pragma once

#include "simParameters.h"
#include "Person.h"
#include "Pandemic_RNG.h"

#include <cstdlib>
#include <cstdio>
#include <map>


class DoctorExperiment
{
public:
	DoctorExperiment(Pandemic_RNG * rng);
	~DoctorExperiment(void);

	void setup_loadConfig();

	void doctor_decideIfWillSeeDoctor(Person * p, int action);
	void doctor_insertIntoQueue(Person * p, int doctor_day, float fraction);
	void daily_doctorExperiment_processSamples(int sim_day, int infectedToday_both, int infectedToday_pandemic, int infectedToday_seasonal);

	Pandemic_RNG * myRNG;
	FILE * fDoctor;

	int lab_capacity;
	float percent_doctor;
	float percent_severe, percent_testing_facility, percent_can_pay;

	int samples_infectedPandemic, samples_infectedSeasonal, samples_infectedBoth;

#define NUMBER_OF_QUEUES MAX_MAX_DAYS + 1 + CULMINATION_PERIOD
	omp_lock_t queue_locks[NUMBER_OF_QUEUES];
	int doctor_infectedBoth[NUMBER_OF_QUEUES];
	int doctor_infectedPandemic[NUMBER_OF_QUEUES];
	int doctor_infectedSeasonal[NUMBER_OF_QUEUES];
	multimap<float,int> sampleBacklog[2];
	multimap<float,Person *> sampleQueue[NUMBER_OF_QUEUES];
};

