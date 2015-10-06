#include "DoctorExperiment.h"


DoctorExperiment::DoctorExperiment(Pandemic_RNG * rng)
{
	//save pointer to the rng
	myRNG = rng;

	//load cfg file
	setup_loadConfig();

	//init OMP queues
	for(int i = 0; i < NUMBER_OF_QUEUES; i++)
	{
		omp_init_lock(&queue_locks[i]);
		doctor_infectedBoth[i] = 0;
		doctor_infectedPandemic[i] = 0;
		doctor_infectedSeasonal[i] = 0;
	}

	//open output files
	fDoctor = fopen("output_doctor_sampling.csv","w");
	fprintf(fDoctor,"day,sampleBacklog2,sampleBacklog1,samplebacklogToday,today_infectedBoth,today_infectedPandemic,today_infectedSeasonal,doctor_infectedBoth,doctor_infectedPandemic,doctor_infectedSeasonal,samples_infectedBoth,samples_infectedPandemic,samples_infectedSeasonal\n");
}


DoctorExperiment::~DoctorExperiment(void)
{
	//close output files
	fclose(fDoctor);
}


void DoctorExperiment::doctor_decideIfWillSeeDoctor(Person * p, int action)
{

	if(action == ACTION_INFECT_PANDEMIC|| action == ACTION_INFECT_BOTH)
	{
		p->severity_pandemic = myRNG->utility_uniformFloat();

	}
	if (action == ACTION_INFECT_SEASONAL || action == ACTION_INFECT_BOTH)
	{
		p->severity_seasonal = myRNG->utility_uniformFloat();
	}


	float doctor_f = myRNG->utility_uniformFloat();

	bool willSeeDoctor = doctor_f < percent_doctor;



	//if we have not returned, assign a doctor day
/*
	if(willSeeDoctor && p->doctor_day < 0)
	{
		float y = utility_uniformFloat();
		auto *profile = &gamma1;
		switch(action)
		{
		case ACTION_INFECT_BOTH:
		case ACTION_INFECT_PANDEMIC:
			switch(p->profile_pandemic)
			{
			case 1:
				//gamma1
				profile = &gamma1;
				break;
			case 2:
				profile = &lognorm1;
				break;
			case 3:
				profile = &weib1;
				break;
			case 4:
				profile = &gamma2;
				break;
			case 5:
				profile = &lognorm2;
				break;
			case 6:
				profile = &weib2;
				break;
			}
			break;

		case ACTION_INFECT_SEASONAL:
			switch(p->profile_seasonal)			
			{
			case 1:
				//gamma1
				profile = &gamma1;
				break;
			case 2:
				profile = &lognorm1;
				break;
			case 3:
				profile = &weib1;
				break;
			case 4:
				profile = &gamma2;
				break;
			case 5:
				profile = &lognorm2;
				break;
			case 6:
				profile = &weib2;
				break;
			}
			break;
		}

		int day;
		for(day = 0; y > (*profile)[day][1] && day < 9; day++)
		{
			y -= (*profile)[day][1];
		}

		//		day = 9;

		float fraction = y / (*profile)[day][1];

		int doctor_day = day + sim_day + 1;
		p->doctor_day = doctor_day;

		doctor_insertIntoQueue(p,doctor_day,fraction);

	}*/
}

void DoctorExperiment::doctor_insertIntoQueue(Person * p, int doctor_day, float fraction)
{

	omp_set_lock(&queue_locks[doctor_day]);
	bool isSevere = false;
	//assign severities
	if(Person::infected_both(p))
	{
		if(p->severity_pandemic < percent_severe || p->severity_seasonal < percent_severe)
			isSevere = true;
		doctor_infectedBoth[doctor_day]++;
	}
	else if(Person::infected_pandemic(p))
	{
		if(p->severity_pandemic < percent_severe)
			isSevere = true;
		doctor_infectedPandemic[doctor_day]++;
	}
	else if(Person::infected_seasonal(p))
	{
		if(p->severity_seasonal < percent_severe)
			isSevere = true;
		doctor_infectedSeasonal[doctor_day]++;
	}
	else
		throw;		//not infected seasonal or pandemic

	float canPay_f = myRNG->utility_uniformFloat();
	float facility_inhouse_f = myRNG->utility_uniformFloat();

	bool canPay = canPay_f < percent_can_pay;
	bool facility_inhouse = facility_inhouse_f < percent_testing_facility;

	bool submitSample = isSevere || (canPay && facility_inhouse);

	std::pair<float,Person *> mapItem(fraction,p);

	if(submitSample)
		sampleQueue[doctor_day].insert(mapItem);

	omp_unset_lock(&queue_locks[doctor_day]);
}

/*
void PandemicSim::submit_sample(Person *p)
{
int sample_day_pandemic = (int) floor(p->doctor_day);
if(days_elapsed() == sample_day_pandemic)
{
float fraction = p->doctor_day - floor(p->doctor_day);
sampleQueue[2].insert(std::pair<float,Person *>(fraction, p));
if((p->status_pandemic == ASYMPTOMATIC || p->status_pandemic == SYMPTOMATIC) &&
(p->status_seasonal == ASYMPTOMATIC || p->status_seasonal == SYMPTOMATIC))
{
#pragma omp critical(doctor_both)
this->doctor_infectedBoth++;
}
else if(p->status_pandemic == ASYMPTOMATIC || p->status_pandemic == SYMPTOMATIC)
{
#pragma omp critical(doctor_pandemic)
this->doctor_infectedPandemic++;
}
else if(p->status_seasonal == ASYMPTOMATIC || p->status_seasonal == SYMPTOMATIC)
{
#pragma omp critical(doctor_seasonal)
this->doctor_infectedSeasonal++;
}
}
}*/

void DoctorExperiment::daily_doctorExperiment_processSamples(int sim_day, int infectedToday_both, int infectedToday_pandemic, int infectedToday_seasonal)
{
	int max = lab_capacity;

	samples_infectedBoth = 0;
	samples_infectedPandemic = 0;
	samples_infectedSeasonal = 0;
	std::multimap<float,int>::iterator backlog_iterator;
	std::multimap<float, Person *>::iterator today_iterator;

	int backlog2 = sampleBacklog[0].size();
	int backlog1 = sampleBacklog[1].size();
	int today = sampleQueue[sim_day].size();

	printf("Sample queue: %d backlogged 2 days, %d backlogged 1 day, %d samples from today\n",
		backlog2, backlog1, today);
	for(int i =0; i < 2; i++)
	{
		if(max <= 0)
			break;
		int count = 0;

		for(backlog_iterator = sampleBacklog[i].begin(); backlog_iterator != sampleBacklog[i].end(); ++backlog_iterator)
		{
			if(max <= 0)
				break;
			int sample = (*backlog_iterator).second;
			if(sample == ACTION_INFECT_BOTH)
				samples_infectedBoth++;
			else if(sample == ACTION_INFECT_PANDEMIC)
				samples_infectedPandemic++;
			else if(sample == ACTION_INFECT_SEASONAL)
				samples_infectedSeasonal++;
			else
				throw;
			max--;
			count++;
		}
		printf("Processed %d samples from %d days ago\n",count, 2 - i);

		if( i != 0)
		{
			if(backlog_iterator == sampleBacklog[i].end())
				sampleBacklog[i].clear();
			else
				sampleBacklog[i].erase(sampleBacklog[i].begin(), backlog_iterator);
		}
	}

	sampleBacklog[0].clear();
	sampleBacklog[0].swap(sampleBacklog[1]);


	int count = 0;		//counts number for this queue
	for(today_iterator = sampleQueue[sim_day].begin(); today_iterator != sampleQueue[sim_day].end(); ++today_iterator)
	{
		Person * p = (*today_iterator).second;

		if(max > 0)
		{
			if(Person::infected_both(p))
				samples_infectedBoth++;
			else if(Person::infected_pandemic(p))
				samples_infectedPandemic++;
			else if(Person::infected_seasonal(p))
				samples_infectedSeasonal++;
			else 
				throw;
			max--;
			count++;
		}
		else
		{
			if(Person::infected_both(p))
			{
				std::pair<float, int> sample_pair((*today_iterator).first,ACTION_INFECT_BOTH);
				sampleBacklog[1].insert(sample_pair);
			}
			else if(Person::infected_pandemic(p))
			{
				std::pair<float, int> sample_pair((*today_iterator).first,ACTION_INFECT_PANDEMIC);
				sampleBacklog[1].insert(sample_pair);
			}
			else if(Person::infected_seasonal(p))
			{
				std::pair<float, int> sample_pair((*today_iterator).first,ACTION_INFECT_SEASONAL);
				sampleBacklog[1].insert(sample_pair);
			}
			else
				throw;
		}

	}
	sampleQueue[sim_day].clear();

	printf("Processed %d samples from today\n",count);

	fprintf(fDoctor,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		sim_day,
		backlog2,
		backlog1,
		today,
		infectedToday_both,
		infectedToday_pandemic,
		infectedToday_seasonal,
		doctor_infectedBoth[sim_day],
		doctor_infectedPandemic[sim_day],
		doctor_infectedSeasonal[sim_day],
		samples_infectedBoth,
		samples_infectedPandemic,
		samples_infectedSeasonal);
	fflush(fDoctor);
}

void DoctorExperiment::setup_loadConfig()
{
	FILE * fin = fopen("doctor_sampling.csv","r");

	//chomp first line (headers)
	char * s = (char *) malloc(500 * sizeof(char));
	fgets(s,500,fin);

	//skip first column
	fscanf(fin,"%*[^,]%*c");

	fscanf(fin, "%d%*c", &lab_capacity);
	fscanf(fin, "%f%*c", &percent_doctor);
	fscanf(fin, "%f%*c", &percent_severe);
	fscanf(fin, "%f%*c", &percent_testing_facility);
	fscanf(fin, "%f", &percent_can_pay);

	fclose(fin);
}
