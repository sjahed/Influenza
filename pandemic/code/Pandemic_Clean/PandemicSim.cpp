#include "PandemicSim.h"
#include <cstdio>
#include "simParameters.h"
//#include <string>
//#include <vector>
#include "Person.h"
#include "Location.h"
//#include <time.h>
#include <conio.h>
#include <ctime>
//#include <map>
//#include <algorithm>
#include <fstream>

//#include <Random123/threefry.h>

//#include <mcstl.h>
#include <math.h>

#include "DailyLocationArray.h"

#include "resource_polling.h"

using namespace std;

#define workplace_location_offset 0
#define household_location_offset number_workplaces

//#define UPDATE_HOUR 19
#define DEBUG 0
#define MAX_THREAD_COUNT 1

#ifdef _MSC_VER
#define VISUAL_STUDIO 1
#else
#define VISUAL_STUDIO 1
#endif

int ABSENTEEISM = 0;
int UNEMPLOYMENT = 0;
const int USE_PREGENERATED_HOME = 1;
const int USE_PREGENERATED_WORK = 1;

int ENABLE_DOCTOR_SAMPLING_EXPERIMENT = 0;

//bool status_update_needed = true;

static int sim_day = 0, sim_hour = 0;
#define day_of_week() (sim_day % 7)
#define is_weekend() (day_of_week() >= 5)

#define DEBUG_LOG_CONTACTS 0

int today_infectedPandemic, today_infectedSeasonal, today_infectedBoth;
int today_infectedHome, today_infectedWork, today_infectedSchool, today_infectedAfterschool, today_infectedErrands;
int today_infected_childrenAge5, today_infected_childrenAge9, today_infected_childrenAge14, today_infected_childrenAge17, today_infected_childrenAge22, today_infected_adults;


/*  //does not currently work
//converts the enumerated days back into a string for output printing
inline const char* day_of_week_string(int day)
{
switch(day)
{
case SUNDAY: return "SUNDAY";
case MONDAY: return "MONDAY";
case TUESDAY: return "TUESDAY";
case WEDNESDAY: return "WEDNESDAY";
case THURSDAY: return "THURSDAY";
case FRIDAY: return "FRIDAY";
case SATURDAY: return "SATURDAY";
default: return "BAD_DAY_ENUM";
}
}
*/


//a few small macros to help with some commonly-used expressions
//these are in-lined in the code, they are purely for readability
//these are ternary expressions, and can be read as an if-then-else statement
//like so: function_name()  (condition ? then : else)

bool file_exists(const char * filename)
{
	std::ifstream ifile(filename);
	return ifile;
}

void PandemicSim::timeSimulation()
{
	clock_t t_start, t_end;	//allocate clock objects
	t_start = clock();

	setupAndRunSim();		//run simulation

	t_end = clock();		//calculate the difference between the clock times
	double elapsed_seconds = (t_end - t_start) / (double) CLOCKS_PER_SEC;
	printf("time: %lf seconds\n",elapsed_seconds);

	size_t memory_bytes_used = get_memory_usage();
	size_t memory_megabytes_used = memory_bytes_used >> 20;

	const char * resource_log_filename = "output_resource_log.csv";

	FILE * fResourceLog;
	bool log_exists = file_exists(resource_log_filename);
	
	if(log_exists)
	{
		fResourceLog = fopen(resource_log_filename,"a");	
	}
	else
	{
		fResourceLog = fopen(resource_log_filename,"w");
		fprintf(fResourceLog, "sim_type,sim_device,people_sim_scale,location_sim_scale,seed,runtime_milliseconds,runtime_seconds,bytes_used,megabytes_used\n");
	}

	double elapsed_milliseconds = elapsed_seconds * 1000;

	if(VISUAL_STUDIO)
		fprintf(fResourceLog,"%s,%s,%lf,%lf,%d,%lf,%lf,%Iu,%Iu\n",
			NAME_OF_SIM_TYPE,NAME_OF_SIM_DEVICE,people_scaling_factor,location_scaling_factor,seed,elapsed_milliseconds,elapsed_seconds,memory_bytes_used, memory_megabytes_used);
	else
		fprintf(fResourceLog,"%s,%s,%lf,%lf,%d,%lf,%lf,%zu,%zu\n",
			NAME_OF_SIM_TYPE,NAME_OF_SIM_DEVICE,people_scaling_factor,location_scaling_factor,seed,elapsed_milliseconds,elapsed_seconds,memory_bytes_used, memory_megabytes_used);

	//fprintf(fTime,"scalingFactor,numberPeople,numberWorkplaces,numberHouseholds,timeInSec\n");
	//fprintf(fTime,"%lf,%d,%d,%d,%lf\n",
	//	SIM_SCALING_FACTOR, number_people, number_workplaces, number_households, diff);

	fclose(fResourceLog);
}

void PandemicSim::setupAndRunSim()
{
	setupSimulation();

	for(sim_day = 0; sim_day < simulation_max_days; sim_day++)
	{

		if(CONSOLE_OUTPUT)
		{
			printf("\n----------------------------\nday %2d\npopulation: %8d\ninfected count: %8d\nPercent infected: %.1f%%\n----------------------------\n",
				sim_day, number_people, infected_array.size(), (float) (100 * infected_array.size()) / number_people);
		}

		generateSchedules();

		output_dailyInfectedStatusCount();

		makeContacts_iterateHours();

//		if(0)
//			infected_watch();

		dailyUpdate();

		output_dailyTransmissions();

		if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
			doctorExperiment->daily_doctorExperiment_processSamples(sim_day,today_infectedBoth,today_infectedPandemic,today_infectedSeasonal);			//process samples for lab capacity experiment
	}

	//		fclose(infectedWatch);
	//		fclose(infectionProb);
	cleanup();
}



void PandemicSim::makeContacts_iterateHours()
{
	for(sim_hour = 0; sim_hour < 24; sim_hour++)
	{
		//check to see whether this is an hour that we need to make contacts in
		//if an infected is queued to make contacts, we need to simulate this hour
		int matters = infectedHourlyQueue[sim_hour].size() > 0;	
		if(!matters)
			continue;

		makeContacts_iterateHourlyQueue(sim_hour);

		//if(0)
		//	makeContacts_printHourlyOutput(currentLocArray);
	}
}

void PandemicSim::makeContacts_iterateHourlyQueue(int hour)
{
	vector<Person *> *infectedQueuePtr = &infectedHourlyQueue[hour];
	for(vector<Person *>::iterator it = infectedQueuePtr->begin(); it != infectedQueuePtr->end(); ++it)
	{
		Person * p = *it;

		int contacts_made = p->contacts_made;
		//printf("contacts made: %d    contacts types%d\n",contacts_made,p->contact_type);
		makeContacts_Person(p,hour, p->contact_ids + contacts_made, p->contact_type + contacts_made);
	}
}

int PandemicSim::makeContacts_Person(Person * p, int hour, Person ** outputIds, int * outputKvals)
{
	//non-infected people do not make contacts
	//if(!Person::is_infected(p))
	//	throw std::runtime_error(std::string("Infected array contains non-infected person"));

	int locId = p->schedule[hour];
	int schedule_type = p->schedule_type[hour];
	int contacts_desired = p->contacts_desired[hour];

	//if there is only one person at this location, we cannot make any valid contacts
	int loc_count = dailyLocArray.getLocCount(locId,is_weekend(),hour);

	if(loc_count <= 1)
		contacts_desired = 0;

	int contacts_made = 0;

	for(int i = 0; i < contacts_desired; i++)
	{
		//if the infector has already made his maximum contacts for the day, stop
		if(p->contacts_made == CONTACTS_CAPACITY)
			break;

		//if there are N people at the location, get a number between 0 and N-1 to select victim
		int victim_offset = myRNG->utility_uniformInt(0, loc_count - 1);
		//printf("low=%d  upper=%d   return=%d\n",0,loc_count-1,victim_offset);
		//get the pointer to the victim
		Person * victim = dailyLocArray.getPersonAtLoc(victim_offset,locId,is_weekend(),hour);

		//if we accidentally drew the same person who is doing the infection, get the next person
		//if we selected the last person in the array, get the first one
		if(victim == p)
		{
			victim_offset = victim_offset + 1;
			victim_offset = victim_offset % loc_count;
			victim = dailyLocArray.getPersonAtLoc(victim_offset,locId,is_weekend(),hour);
		}


		//store the victim and the k-factor
		outputIds[contacts_made] = victim;
		outputKvals[contacts_made] = schedule_type;

		contacts_made++;
		p->contacts_made++;
		p->cumulative_k += k_values[schedule_type];
	}

	return contacts_made;
}



void PandemicSim::dailyUpdate()
{
	//#pragma omp parallel reduction(+:today_infectedPandemic,today_infectedSeasonal,today_infectedBoth, today_infectedHome, today_infectedWork, today_infectedSchool, today_infectedErrands, today_infectedAfterschool, today_infected_childrenAge5, today_infected_childrenAge9, today_infected_childrenAge14, today_infected_childrenAge17,today_infected_adults)
	{
		//store your random number back into the buffer
		//int thread_no = omp_get_thread_num();
		//rand_buff_idx = rand_buff_idx_store[thread_no];

		//generate contacts, if the hour is right then do updates
		//	#pragma omp for schedule(dynamic) 

		//everyone makes contacts during the home hour of the day, so we'll use this as a makeshift infected_array
		//int hourToIterate = weekend() ? HOUR_WEEKEND_HOME : HOUR_WEEKDAY_HOME;

		for(vector<Person *>::iterator it = infected_array.begin();  it != infected_array.end(); ++it)
		{
			daily_makeInfections(*it);
			daily_recoverIfCulminated(*it);
		}

		//rand_buff_idx_store[thread_no] = rand_buff_idx;
	}
}

void PandemicSim::generateSchedules()
{
	//clear infected array
	infected_array.clear();

	//clear hourly queues
	for(int i = 0; i < 24; i++)
		infectedHourlyQueue[i].clear();

	//clear the daily location array
	dailyLocArray.Empty();

	//clear daily status counters
	for(int i = 0; i < 4; i++)
	{
		count_pandemic_status[i] = 0;
		count_seasonal_status[i] = 0;
	}

	if(is_weekend())
	{
		//#pragma omp parallel for 
		for(int i = 0; i < number_people; i++)
		{
			schedule_personWeekend(i);
			schedule_countPersonStatus(i);
		}
	}
	else
	{
		//#pragma omp parallel for
		for(int i = 0; i < number_people; i++)
		{
			schedule_personWeekday(i);
			schedule_countPersonStatus(i);
		}
	}

}



PandemicSim::PandemicSim()
{
	today_infectedBoth = 0;
	today_infectedPandemic = 0;
	today_infectedSeasonal = 0;

	today_infectedAfterschool = 0;
	today_infectedErrands = 0;
	today_infectedHome = 0;
	today_infectedSchool = 0;
	today_infectedWork = 0;

	today_infected_childrenAge5 = 0;
	today_infected_childrenAge9 = 0;
	today_infected_childrenAge14 = 0;
	today_infected_childrenAge17 = 0;
	today_infected_adults = 0;


	//default values
	number_households = 5000;
	number_workplaces = 1280;
	number_locations = number_households + number_workplaces;
	location_scaling_factor = 1.;
	people_scaling_factor = 1.;

	//zero fill output arrays
	for(int i = 0; i < MAX_MAX_DAYS; i++)
	{
		reproduction_pandemic[i] = 0;
		reproduction_seasonal[i] = 0;
	}
}


PandemicSim::~PandemicSim(void)
{
}

FILE * output_stats_file;



int PandemicSim::schedule_getErrandDestination()
{
	double sum = 0, y = myRNG->utility_uniformFloat();
	int column;

	const int first_row = 9;	//cheat:  The first business type with any probability of being an errand is type 9

	//select column index: if a weekday, we search the weekday percentages, and vice versa
	if(is_weekend())
		column = DATA_WORKPLACE_WEEKEND_ERRAND_PERCENT_IDX;
	else
		column = DATA_WORKPLACE_WEEKDAY_ERRAND_PERCENT_IDX;

	//	int total_offset = workplace_location_offset;  //all locations stored together, this indexes to the first one
	for(int business_type = first_row; business_type < number_business_types; business_type++)
	{
		//add the percent of businesses of this type to our sum
		sum += workplace_data[business_type][column];

		//if the sum is greater than the y-value, the business is of this type
		if(sum > y|| business_type == number_business_types - 1)
		{
			sum -= y;  //sum now contains the amount the roll "overshot" the threshold
			sum = sum / workplace_data[business_type][column];  //Now contains a proportion, i.e. .66 or 2/3 of the way through this type
			sum = sum * workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX];  //now contains an actual index, double format
			return (int) floor(sum) + workplace_location_offset + workplace_type_cumulative_count[business_type]; //floor it, add it to the running offset count we've got going, return
		}
		//		else  //if not, store the number of workplaces we've skipped over
		//			total_offset += (int) workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX];
	}

	throw std::exception("error in errand PDF function");
}

void PandemicSim::schedule_threeErrands(Person * p, int low_hour, int high_hour)
{
	//DOES NOT put them into the location array

	int i, errand_hour_array[3]; 

	//TODO: FIX
	//get 3 unique errand hours
	errand_hour_array[0] = myRNG->utility_uniformInt(low_hour, high_hour);
	do{errand_hour_array[1] = myRNG->utility_uniformInt(low_hour, high_hour);}
	while (errand_hour_array[1] == errand_hour_array[0]);   //generate random numbers until we get 3 unique ones
	do{errand_hour_array[2] = myRNG->utility_uniformInt(low_hour, high_hour);}
	while (errand_hour_array[2] == errand_hour_array[1] || errand_hour_array[2] == errand_hour_array[0]);

	//assign 2 contacts between the three hours
	int errand_contacts_desired[3];
	i = myRNG->utility_uniformInt(0,5);
	switch(i)
	{
	case 0:
		errand_contacts_desired[0] = 2;
		errand_contacts_desired[1] = 0;
		errand_contacts_desired[2] = 0;
		break;
	case 1:
		errand_contacts_desired[0] = 0;
		errand_contacts_desired[1] = 2;
		errand_contacts_desired[2] = 0;
		break;
	case 2:
		errand_contacts_desired[0] = 0;
		errand_contacts_desired[1] = 0;
		errand_contacts_desired[2] = 2;
		break;
	case 3: 
		errand_contacts_desired[0] = 1;
		errand_contacts_desired[1] = 1;
		errand_contacts_desired[2] = 0;
		break;
	case 4: 
		errand_contacts_desired[0] = 1;
		errand_contacts_desired[1] = 0;
		errand_contacts_desired[2] = 1;
		break;
	case 5: 
		errand_contacts_desired[0] = 0;
		errand_contacts_desired[1] = 1;
		errand_contacts_desired[2] = 1;
		break;
	}

	//store the data for the person
	for(i = 0; i < 3; i++){
		int errand_destination_idx = schedule_getErrandDestination();
		int hour = errand_hour_array[i];

		p->schedule[hour] = errand_destination_idx;
		p->schedule_type[hour] = ERRAND;
		p->contacts_desired[hour] = errand_contacts_desired[i];

		dailyLocArray.insertPerson(p,errand_destination_idx,is_weekend(),hour);
	}

	//if they are infected, mark them to make contacts this hour
	if(Person::is_infected(p))
	{
		infectedHourlyQueue[errand_hour_array[0]].push_back(p);
		infectedHourlyQueue[errand_hour_array[1]].push_back(p);
		infectedHourlyQueue[errand_hour_array[2]].push_back(p);
	}

	
}

void PandemicSim::schedule_personWeekday(int personIdx)
{
	Person * p = &peopleArray[personIdx];

	//default:  person is home
	for(int i = 0; i < 24; i++){
		p->schedule[i] = p->household;
		p->schedule_type[i] = HOUSEHOLD;
		p->contacts_desired[i] = 0;
	}

	//setup home hour
	p->contacts_desired[HOUR_HOME] = HOUSEHOLD_MAX_CONTACTS;

	//if we are not using pre-generated home arrays, schedule and build this hour
	if(!USE_PREGENERATED_HOME)
		dailyLocArray.insertPerson(p, p->household, 0, HOUR_HOME);

	if(Person::is_infected(p))
		infectedHourlyQueue[HOUR_HOME].push_back(p);

	if(Person::is_infected(p))
		infected_array.push_back(p);

	//if absenteeism is turned on and the person has an active, symptomatic infection,
	//they will stay at home all day
	if(ABSENTEEISM && (p->status_pandemic == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_SYMPTOMATIC))
		return;

	//adults
	if(p->age > 22)	
	{
		//unemployed will go on 3 errands during the specified time window

		if(UNEMPLOYMENT)
			throw std::exception();	
		//std::string("not implemented")

		//in this case, we must generate locationarrays for each hour rather than only the ones that matter
		//	if(p->unemployed)
		//		schedule_errands(p, HOUR_UNEMPLOYED_ERRAND_START, HOUR_UNEMPLOYED_ERRAND_END);

		//employed
		else
		{
			//work during the day
			for(int i = HOUR_WORK_START; i <= HOUR_WORK_END; i++)
			{
				p->schedule[i] = p->workplace;
				p->schedule_type[i] = BUSINESS;
			}

			p->contacts_desired[HOUR_WORK_START] = dailyLocArray.locData[p->workplace].max_contacts;

			//if we are not using pregenerated work arrays, schedule and build this hour
			if(!USE_PREGENERATED_WORK)
				dailyLocArray.insertPerson(p,p->workplace,0,HOUR_WORK_START);

			if(Person::is_infected(p))
				infectedHourlyQueue[HOUR_WORK_START].push_back(p);

			//then go on two errands right after work
			int errand_idx_1 = schedule_getErrandDestination();
			p->schedule[HOUR_WORK_END + 1] = errand_idx_1;
			p->schedule_type[HOUR_WORK_END + 1] = ERRAND;
			dailyLocArray.insertPerson(p,errand_idx_1, 0, HOUR_WORK_END + 1);
		//	printf("***********************errand idx= %d",errand_idx_1);
		//
		//	getch();
			int errand_idx_2 = schedule_getErrandDestination();
			p->schedule[HOUR_WORK_END + 2] = errand_idx_2;
			p->schedule_type[HOUR_WORK_END + 2] = ERRAND;
			dailyLocArray.insertPerson(p, errand_idx_2, 0, HOUR_WORK_END+2);

			//assign 2 contacts split between these errands
			int contacts = myRNG->utility_uniformInt(0,2);
			p->contacts_desired[HOUR_WORK_END + 1] = contacts;
			p->contacts_desired[HOUR_WORK_END + 2] = 2 - contacts;

			if(Person::is_infected(p))
			{
				infectedHourlyQueue[HOUR_WORK_END + 1].push_back(p);
				infectedHourlyQueue[HOUR_WORK_END + 2].push_back(p);
			}
		}
	}

	//children
	else     
	{
		//children go to school during the day
		for(int i = HOUR_SCHOOL_START; i <= HOUR_SCHOOL_END; i++)
		{
			p->schedule[i] = p->workplace;
			p->schedule_type[i] = SCHOOL;
		}

		p->contacts_desired[HOUR_SCHOOL_START] = dailyLocArray.locData[p->workplace].max_contacts;

		//if we are not using pregenerated work arrays, schedule and build this hour
		if(!USE_PREGENERATED_WORK)
			dailyLocArray.insertPerson(p, p->workplace, 0,HOUR_SCHOOL_START);

		if(Person::is_infected(p))
			infectedHourlyQueue[HOUR_SCHOOL_START].push_back(p);

		//and then select a random afterschool location which they will attend for 3 hours
		//if there are N afterschool locations, select one between 0 and N-1
		int afterschool_location = myRNG->utility_uniformInt(0, (int) workplace_data[DATA_BUSINESS_TYPE_AFTERSCHOOL_IDX][DATA_WORKPLACE_TYPE_COUNT_IDX] - 1);

		//add the offset of the location type to the selected afterschool location
		afterschool_location += workplace_type_cumulative_count[DATA_BUSINESS_TYPE_AFTERSCHOOL_IDX];

		//mark the hours as afterschool
		for(int hour = HOUR_AFTERSCHOOL_START; hour <= HOUR_AFTERSCHOOL_END; hour++)
		{
			p->schedule[hour] = afterschool_location;
			p->schedule_type[hour] = AFTERSCHOOL;
			dailyLocArray.insertPerson(p, afterschool_location, 0,hour);
		}
		if(Person::is_infected(p))
			infectedHourlyQueue[HOUR_AFTERSCHOOL_START].push_back(p);
	}
}

void PandemicSim::schedule_personWeekend(int personIdx)
{
	Person * p = &peopleArray[personIdx];

	//default: schedule them at home all day
	for(int i = 0; i < 24; i++)
	{
		p->schedule[i] = p->household;
		p->schedule_type[i] = HOUSEHOLD;
		p->contacts_desired[i] = 0;
	}

	//setup home hour
	p->contacts_desired[HOUR_HOME] = HOUSEHOLD_MAX_CONTACTS;

	//if we are not using pregenerated home arrays, schedule and build this hour
	if(!USE_PREGENERATED_HOME)
		dailyLocArray.insertPerson(p, p->household, 1,HOUR_HOME);

	//setup home hour contacts
	if(Person::is_infected(p))
		infectedHourlyQueue[HOUR_HOME].push_back(p);

	//build the global infected array
	if(Person::is_infected(p))
		infected_array.push_back(p);

	//if absenteeism is on and they are currently symptomatic on at least one disease, they will stay home all day
	if(ABSENTEEISM && (p->status_pandemic == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_SYMPTOMATIC))
		return;

	//otherwise they will run 3 errands in the defined time period
	schedule_threeErrands(p, HOUR_WEEKEND_ERRAND_START, HOUR_WEEKEND_ERRAND_END);
}


void PandemicSim::setReproductionNumber(Person * p, int action)
{
	if(action == ACTION_INFECT_PANDEMIC)
	{
		if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
			doctorExperiment->doctor_decideIfWillSeeDoctor(p,action);

		//if the person currently has seasonal, adjust both flu reproduction by the cross-epsilon factors
		if(p->status_seasonal == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_ASYMPTOMATIC){
			p->reproduction_initial_pandemic = reproduction_number_pandemic * epsilon_ps;
			//				p->reproduction_initial_seasonal = reproduction_number_seasonal * epsilon_sp;
			//			hourly_infected_coinfected_sp_count++;
		}
		//if the person is recovered, adjust the reproduction by the immunity-epsilon factor
		else if (p->status_seasonal == STATUS_RECOVERED){
			p->reproduction_initial_pandemic = reproduction_number_pandemic * epsilon_p;
			//			hourly_reinfected_sp_count++;
		}
		else{
			//else simply store the reproduction number
			p->reproduction_initial_pandemic = reproduction_number_pandemic;
			//			hourly_infected_pandemic_count++;
		}
		//pragma omp critical(today_infections_pandemic)
		today_infectedPandemic++;
	}

	else if (action == ACTION_INFECT_SEASONAL)
	{
		if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
			doctorExperiment->doctor_decideIfWillSeeDoctor(p,action);

		//if thep erson currently has pandemic, adjust both reproductions by the cross-immunity factors
		if(p->status_pandemic == STATUS_SYMPTOMATIC || p->status_pandemic == STATUS_ASYMPTOMATIC)
		{
			//			p->reproduction_initial_pandemic = reproduction_number_pandemic * epsilon_ps;
			p->reproduction_initial_seasonal = reproduction_number_seasonal * epsilon_sp;
			//			hourly_infected_coinfected_ps_count++;
		}
		else if (p->status_pandemic == STATUS_RECOVERED)
		{
			//if the person is recovered, adjust the reproduction by the immunity-epsilon factor
			p->reproduction_initial_seasonal = reproduction_number_seasonal * epsilon_s;
			//			hourly_reinfected_ps_count++;
		}
		else 
		{
			//else simply store the reproduction number
			p-> reproduction_initial_seasonal = reproduction_number_seasonal;
			//			hourly_infected_seasonal_count++;
		}
		//#pragma omp critical(today_infections_seasonal)
		today_infectedSeasonal++;
	}

	else if (action == ACTION_INFECT_BOTH){
		if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
			doctorExperiment->doctor_decideIfWillSeeDoctor(p,action);

		//store both reproduction numbers adjusted by the cross-immunity factors
		p->reproduction_initial_pandemic = reproduction_number_pandemic * epsilon_ps;
		p->reproduction_initial_seasonal = reproduction_number_seasonal * epsilon_sp;
		//		hourly_infected_coinfected_count++;	
		//#pragma omp critical(today_infections_both)
		today_infectedBoth++;
	}

	else if (action == ACTION_RECOVER_PANDEMIC){
		//if the person still has an active seasonal infection, modify the seasonal reproduction
		if(p->status_seasonal == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_ASYMPTOMATIC){
			p->reproduction_initial_seasonal = reproduction_number_seasonal * epsilon_s;}}

	else if (action == ACTION_RECOVER_SEASONAL){
		//if the person still has an active pandemic infection, modify the pandemic reproduction
		if(p->status_pandemic == STATUS_SYMPTOMATIC || p->status_pandemic == STATUS_ASYMPTOMATIC){
			p->reproduction_initial_pandemic = reproduction_number_pandemic * epsilon_p;}}
}


void PandemicSim::dump_contact(int person, 
							   int day_p, double p_p, 
							   int day_s, double p_s, 
							   float k_val, double z_p, double z_s)
{
	int status_p = peopleArray[person].status_pandemic;
	int status_s = peopleArray[person].status_seasonal;

	int profile_p = peopleArray[person].profile_pandemic;
	int profile_s = peopleArray[person].profile_seasonal;

	int success_p = p_p * k_val >= z_p;
	int success_s = p_s * k_val >= z_s;

	fprintf(fContacts,
		"%d, %d, %f, %d, %d, %d, %lf, %lf, %d, %d, %d, %d, %lf, %lf, %d\n",
		sim_day, person, k_val,
		status_p, day_p, profile_p, p_p, z_p, success_p,
		status_s, day_s, profile_s, p_s, z_s, success_s);
}

void PandemicSim::daily_makeInfections(Person * infector)
{
	//option: select contacts here
	/*for(int h = 0; h < 24; h++)
	{
		int contacts_this_hour = infector->contacts_desired[h];
		if(contacts_this_hour > 0)
		{
			LocationArray * array = makeContacts_getLocArray(day_of_week,h);

			int locId = infector->schedule[h];
			Location * l = array->getLocation(locId);

			makeContacts_Person(infector,l,contacts_this_hour);
		}
	}*/




	//As we iterate through the array, a person may receive a second strain of infection
	//before their own contacts are processed.  This would mean they are on day -1 of an infection.
	//This determines whether they had a particular strain on the current day

	infector->infections_today = 0;

	bool active_pandemic = (infector->status_pandemic == STATUS_SYMPTOMATIC || infector->status_pandemic == STATUS_ASYMPTOMATIC) 
		&& infector->day_pandemic_infection <= sim_day;
	bool active_seasonal = (infector->status_seasonal == STATUS_SYMPTOMATIC || infector->status_seasonal == STATUS_ASYMPTOMATIC) 
		&& infector->day_seasonal_infection <= sim_day;

	if(active_pandemic || active_seasonal)
	{
		double p_p = -1, p_s = -1, z_p, z_s;

		//fish out the days since this infection began
		int pandemic_infection_day = -1;				//variable pulled up for debug printing below
		if(active_pandemic)
		{
			//day of infection between 0 and 9
			int pandemic_infection_day = sim_day - infector->day_pandemic_infection;

			if(pandemic_infection_day >= CULMINATION_PERIOD)
				printf("ERROR: pandemic infection day past culmination\n");

			p_p = daily_calculateInfectionThreshold(infector, infector->reproduction_initial_pandemic, infector->profile_pandemic, pandemic_infection_day);
		}

		//fish out the days since this infection began
		int seasonal_infection_day = -1;
		if(active_seasonal)								//variable pulled up for debug printing below
		{
			//day of infection between 0 and 9
			seasonal_infection_day = sim_day - infector->day_seasonal_infection;

			if(seasonal_infection_day >= CULMINATION_PERIOD)
				printf("ERROR: seasonal infection day past culmination\n");

			p_s = daily_calculateInfectionThreshold(infector, infector->reproduction_initial_seasonal, infector->profile_seasonal, seasonal_infection_day);
		}

		for(int i = 0; i < infector->contacts_made; i++)
		{
			int loc_type = infector->contact_type[i];
			float k_val = k_values[loc_type];
			Person * victim = infector->contact_ids[i];

			//get our y-value/roll
			z_p = myRNG->utility_uniformFloat();
			z_s = myRNG->utility_uniformFloat();


			if(DEBUG_LOG_CONTACTS)
			{
				int id = infector - &peopleArray[0];

				dump_contact(id, pandemic_infection_day, p_p, seasonal_infection_day, p_s, k_val, z_p, z_s);
			}

			float p_p_adjusted = p_p * k_val;
			float p_s_adjusted = p_s * k_val;

			//if the y-value meets the probability threshold, transmit the diesase
			if(p_p_adjusted >= z_p  && victim->status_pandemic == STATUS_UNEXPOSED)
			{
				//dual infection
				if(p_s_adjusted >= z_s && victim->status_seasonal == STATUS_UNEXPOSED)  //if both values exceeded threshold, 
				{
					reproduction_pandemic[infector->generation_pandemic]++;
					reproduction_seasonal[infector->generation_seasonal]++;

					transmitInfection_seasonal(victim);
					setReproductionNumber(victim, ACTION_INFECT_BOTH);

					victim->generation_pandemic = infector->generation_pandemic + 1;
					victim->generation_seasonal = infector->generation_seasonal + 1;
				}

				//only pandemic infection
				else
				{
					reproduction_pandemic[infector->generation_pandemic]++;

					transmitInfection_pandemic(victim);
					setReproductionNumber(victim, ACTION_INFECT_PANDEMIC);

					victim->generation_pandemic = infector->generation_pandemic + 1;
				}

				infector->infections_today++;
				count_age_and_location(infector->age, loc_type);

			} 
			else if(p_s_adjusted >= z_s && victim->status_seasonal == STATUS_UNEXPOSED)
			{
				reproduction_seasonal[infector->generation_seasonal]++;

				transmitInfection_seasonal(victim);
				setReproductionNumber(victim,ACTION_INFECT_SEASONAL);

				victim->generation_seasonal = infector->generation_seasonal + 1;

				infector->infections_today++;
				count_age_and_location(infector->age, loc_type);
			}
		} //end contacts loop
	} //end of people infected

	//each day, the contact rates reset to zero
	//	p->contact_rate_er = 0;
	//	p->contact_rate_hh = 0;
	//	p->contact_rate_wp = 0;
	//	p->contact_rate_total = 0;
	infector->contacts_made = 0;
	infector->cumulative_k = 0.0;
}



void PandemicSim::daily_recoverIfCulminated(Person * p)
{
	//after the culmination period expires, the person recovers from the disease
	if(p->status_pandemic == STATUS_SYMPTOMATIC || p->status_pandemic == STATUS_ASYMPTOMATIC)
	{
		//if the next day will be greater than the culmination period, recover
		if((sim_day + 1) - p->day_pandemic_infection  >= CULMINATION_PERIOD)
		{
			p->status_pandemic = STATUS_RECOVERED;
			setReproductionNumber(p, ACTION_RECOVER_PANDEMIC);

			//if a given person was infected and has cleared all their infections, remove
			//them from the list of active infected
			//			if(!Person::is_infected(p))
			//				infected_to_remove.emplace_back(p);
		}
	}

	if(p->status_seasonal == STATUS_SYMPTOMATIC || p->status_seasonal == STATUS_ASYMPTOMATIC)
	{
		if((sim_day + 1) - p->day_seasonal_infection >= CULMINATION_PERIOD)
		{
			p->status_seasonal = STATUS_RECOVERED;
			setReproductionNumber(p, ACTION_RECOVER_SEASONAL);	

			//if a given person was infected and has cleared all their infections, remove
			//them from the list of active infected
			//			if(!Person::is_infected(p))
			//				infected_to_remove.emplace_back(p);
		}
	}
}


FILE * infectionProb;

double PandemicSim::daily_calculateInfectionThreshold (Person * p, float repnum, int profile, int infection_day)
{
	//lifted nearly verbatim from the original code


	double a,prob;
	switch (profile)
	{
	case 1:
		a = gamma1[infection_day][1];
		//prob = ((repnum*a*(1+((1-asymp)/((percent_symptomatic)/(1-percent_symptomatic)))))/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er))); This used to go for case 1, case 2 and case 3
		break;
	case 2:
		a = lognorm1[infection_day][1];
		break;
	case 3:
		a = weib1[infection_day][1];
		break;
	case 4:
		a = gamma2[infection_day][1];
		break;
	case 5:
		a = lognorm2[infection_day][1];
		break;
	case 6:
		a = weib2[infection_day][1];
		break;
	}

	prob = ((repnum/(((1.0 - asymp_factor)*percent_symptomatic)+asymp_factor))*a);
	if(profile == 4 || profile == 5 || profile == 6)
		prob *= asymp_factor;

	prob /= p->cumulative_k;
	//	double expected = a * reproduction_number_pandemic / (double) p->contact_rate_total;
	//	if(abs((expected - prob) / prob) > 0.05)
	//		printf("");

	return prob;
}

/*void PandemicSim::insertVictimToInfectedArray(Person *  victim)
{
if(!Person::is_infected(victim))
{
infected_new_today.emplace_back(victim);
}
}*/



void PandemicSim::transmitInfection_pandemic(Person * victim)
{
	victim->day_pandemic_infection = sim_day + 1;

	//types 1-3 represent symptomatic profiles.  
	//Note that these are not array indeces, they are caught by a switch block
	if(myRNG->utility_uniformFloat() <= percent_symptomatic)
	{
		victim->status_pandemic = STATUS_SYMPTOMATIC;
		victim->profile_pandemic = myRNG->utility_uniformInt(1,3);
		victim->symptomatic_pandemic = true;
	}

	//types 4-6 represent asymptomatic profiles
	else
	{
		victim->status_pandemic = STATUS_ASYMPTOMATIC;
		victim->profile_pandemic = myRNG->utility_uniformInt(4,6);
		victim->symptomatic_pandemic = false;
	}
}
void PandemicSim::transmitInfection_seasonal(Person * victim)
{
	victim->day_seasonal_infection = sim_day + 1;

	//types 1-3 represent symptomatic profiles
	if(myRNG->utility_uniformFloat() <= percent_symptomatic)
	{
		victim->status_seasonal = STATUS_SYMPTOMATIC;
		victim->profile_seasonal = myRNG->utility_uniformInt(1,3);
		victim->symptomatic_seasonal = true;
	}
	else
	{
		//types 4-6 represent asymptomatic profiles
		victim->status_seasonal = STATUS_ASYMPTOMATIC;
		victim->profile_seasonal = myRNG->utility_uniformInt(4,6);
		victim->symptomatic_seasonal = false;
	}
}


void PandemicSim::cleanup_calculateReproduction()
{
	//	char * line_buffer = (char *) malloc(TXT_BUFF_SIZE);
	//	str_vector * output_header = split("Generation,Generation_size_p,Avg_R_p,Generation_size_s,AvgR_s", ',');
	//	output_R = init_vecofvec();
	//	vecofvec_push(output_R,output_header);
	FILE * output_R = fopen("output_rn.txt", "w");
	fprintf(output_R,"Generation,Generation_size_p,Avg_R_p,Generation_size_s,Avg_R_s\n", ',');

	int gen_size_pandemic = initial_infected_pandemic;
	int gen_size_seasonal = initial_infected_seasonal;

	for(int gen = 0; gen < MAX_MAX_DAYS - 1; gen++)
	{
		int next_gen_size_pandemic = reproduction_pandemic[gen];
		float rn_pandemic = (float) next_gen_size_pandemic / gen_size_pandemic;

		int next_gen_size_seasonal = reproduction_seasonal[gen];
		float rn_seasonal = (float) next_gen_size_seasonal / gen_size_seasonal;

		fprintf(output_R, "%d,%d,%f,%d,%f\n",
			gen,
			gen_size_pandemic, rn_pandemic,
			gen_size_seasonal, rn_seasonal);

		gen_size_pandemic = next_gen_size_pandemic;
		gen_size_seasonal = next_gen_size_seasonal;
	}


	fclose(output_R);

}

/*void PandemicSim::cleanup_calculateReproductionBySymptomatic()
{
	FILE * output_R = fopen("output_R_symptomatic.csv", "w");
	fprintf(output_R,"Generation,generation_size_p,generation_size_p_symptomatic,generation_size_p_asymptomatic,rn,rn_symptomatic,rn_asymptomatic\n", ',');

	for(int gen = 1; gen < simulation_max_days; gen++)
	{
		int symptomatic_sum = 0; 
		int asymptomatic_sum = 0;
		int symptomatic_count = 0;
		int asymptomatic_count = 0;

		for(int i = 0; i < number_people; i++){
			Person * p = &peopleArray[i];
			if(p->generation_pandemic == gen){
				if(p->symptomatic_pandemic == true)
				{
					symptomatic_count++;
					symptomatic_sum += p->reproduction_number_pandemic;
				}
				else
				{
					asymptomatic_count++;
					asymptomatic_sum += p->reproduction_number_pandemic;
				}}
		}

		int generation_size = asymptomatic_count + symptomatic_count;
		float global_rn = (float) (asymptomatic_sum + symptomatic_sum) / generation_size;

		float symptomatic_rn = (float) symptomatic_sum / symptomatic_count;
		float asymptomatic_rn = (float) asymptomatic_sum / asymptomatic_count;

		fprintf(output_R,"%d,%d,%d,%d,%f,%f,%f\n", 
			gen, generation_size, symptomatic_count,asymptomatic_count,
			global_rn, symptomatic_rn,asymptomatic_rn );
	}
	fclose(output_R);

}*/


void PandemicSim::cleanup()
{
	cleanup_calculateReproduction();
//	cleanup_calculateReproductionBySymptomatic();
//	fclose(output_stats_file);
	if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
		fclose(sampling_data);

	fclose(fDailyInfectionCounts);

	fclose(fInfectedStatistics);

	if(DEBUG_LOG_CONTACTS)
		fclose(fContacts);
}

FILE * infectedWatch, * debug_asserts;
/*
void PandemicSim::infected_watch()
{
	for(int i = 0; i < number_people; i++)
	{
		Person * p = &peopleArray[i];
		if(p->status_pandemic == SYMPTOMATIC || p->status_pandemic == ASYMPTOMATIC || p->status_seasonal == SYMPTOMATIC || p->status_seasonal == ASYMPTOMATIC)
		{
			fprintf(infectedWatch, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%d,%d,%d,%d\n", 
				sim_day,
				p->age, p->household, p->workplace, 
				p->status_pandemic, p->status_seasonal, 
				p->generation_pandemic,p->generation_seasonal, 
				p->day_pandemic_infection,p->day_seasonal_infection, 
				p->profile_pandemic, p->profile_seasonal, 
				p->reproduction_initial_pandemic, p->reproduction_initial_seasonal, 
//				p->reproduction_number_pandemic, p->reproduction_number_seasonal, 
				p->contacts_made, p->infections_today);
			p->infections_today = 0;
		}
	}
	fflush(infectedWatch);
}*/

void PandemicSim::makeContacts_printHourlyOutput(LocationArray * locs)
{
	long count_infected = 0, count_exposed = 0, submitted_sample = 0;


	//	for(int i = 0; i < number_people; i++)
	//		if(Person::is_infected(&peopleArray[i]))
	//			count_infected++;

	count_infected = infected_array.size();


	int household_count_total = 0;
	int business_count_total = 0;

	for(int i = 0; i < locs->locs.size(); i++)
	{
		if(dailyLocArray.locData[i].location_type == BUSINESS)
			business_count_total += locs->locs[i].count_hourly_total;
		else
			household_count_total += locs->locs[i].count_hourly_total;
	}

	long count = household_count_total + business_count_total;
	double household_count_average = (double) household_count_total / (double) number_households;
	double business_count_average = (double) business_count_total / (double) number_workplaces;

	printf("day %d, weekday %d, hour %d, %d people, %d infected, %d exposed\n", sim_day, day_of_week(), sim_hour, count, count_infected, count_exposed);
	printf("average of %2f people per household, %2f per business \n",household_count_average, business_count_average);


	fflush(infectionProb);
	fflush(sampling_data);
	if(DEBUG_LOG_CONTACTS)
		fflush(fContacts);
}

void PandemicSim::setup_generateBusinesses(DailyLocationArray * dailyLocArray)
{
	int workplace_count = 0;
	for(int business_type = 0; business_type < number_business_types; business_type++)
	{
		int type_count = (int) workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX];
		int type_max_contacts = (int) workplace_data[business_type][DATA_WORKPLACE_MAX_CONTACTS_IDX];

		for(int idx = workplace_count + workplace_location_offset; 
			idx < workplace_count + workplace_location_offset + type_count;
			idx++)
		{
			dailyLocArray->locData[idx].location_type = BUSINESS;
			dailyLocArray->locData[idx].business_type = business_type;
			dailyLocArray->locData[idx].max_contacts = type_max_contacts;
			//
			//printf("Loc_type %d idx %d type %d    type_max_contacts %d \n",BUSINESS,idx,business_type,type_max_contacts);
			
		}
		//getch();
		workplace_count += type_count;
	}
}

int PandemicSim::setup_generatePeople_assignAdultWorkplace()
{
	double sum = 0, y = myRNG->utility_uniformFloat();
	int total_offset = workplace_location_offset;  //all locations stored together, this indexes to the first business
	
	//a faster way to fish the business type out
	for(int business_type = 0; business_type < number_business_types; business_type++)
	{
		//add the percent value for this type of business
		sum += workplace_data[business_type][DATA_WORKPLACE_WORK_PERCENT_IDX];
		//printf("sum %f  y %f   offset %d     ",sum,y,total_offset);
		//if the sum is greater than our roll, this person works at this business type
		if(sum > y)
		{
			sum -= y;  //sum now contains the amount the roll "overshot" the threshold
			sum = sum / workplace_data[business_type][DATA_WORKPLACE_WORK_PERCENT_IDX];  //Now contains a proportion, i.e. .66 or 2/3 of the way through this type
			sum = sum * workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX];  //now contains the index, i.e. business 23 of type 2
			return (int) floor(sum) + total_offset; //add it to the total number of workplaces skipped and return
		}
		else  //if not, store the number of workplaces we've skipped over
			total_offset += (int) workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX];
	}
	//printf("error in workplace PDF\n");

	return 0;
}

void PandemicSim::setup_generatePeople_assignChildAgeAndSchool(Person * p)
{
	float y = myRNG->utility_uniformFloat();
	int i = 0;

	//NOTE: Assumes the CDF sums properly to 1
	while(y > child_age_data[i][DATA_CHILD_AGE_CDF_IDX])  //seek to the right age from the CDF
		i++;

	p->age = child_age_data[i][DATA_CHILD_AGE_AGEVAL];  //store the age, fish out the school typeid
	int workplace_type = child_age_data[i][DATA_CHILD_AGE_SCHOOL_TYPE_IDX];

	int offset_total = workplace_location_offset;  //sum up the workplace offset plus the offset from all other business types
	for(i = 0; i < workplace_type; i++)
		offset_total += workplace_data[i][DATA_WORKPLACE_TYPE_COUNT_IDX];
	offset_total += myRNG->utility_uniformInt(0, workplace_data[workplace_type][DATA_WORKPLACE_TYPE_COUNT_IDX] - 1);  //randomly select a school of this type, add hte offset
	p->workplace = offset_total;  //store the school
}


void PandemicSim::setup_generatePeople(DailyLocationArray * dailyLocArray)
{
	number_people = 0;
	//I did the math, and 2.5 is the expected value of number of people per household
	long array_size = 2.5 * number_households * number_population_center;
	peopleArray.resize(array_size);

	for(int pop_center = 0; pop_center < number_population_center; pop_center++)
	{
		for(int i = 0; i < number_households; i++)
		{
			int num_children, num_adults;

			//find the household type
			float y = myRNG->utility_uniformFloat();
			for(int j = 0; j < number_household_types; j++)
			{
				if(household_data[j][DATA_HOUSEHOLD_CDF_IDX] >= y)
				{
					num_adults = household_data[j][DATA_HOUSEHOLD_NUMBER_ADULTS_IDX];
					num_children = household_data[j][DATA_HOUSEHOLD_NUMBER_CHILDREN_IDX];
					break;
				}
			}
				

			if(peopleArray.size() < number_people + num_adults + num_children)
			{
				peopleArray.resize(number_people + 500);
			//	printf("number_peoble %d\n",number_people);
				//getch();
			}
			int household_index = i + household_location_offset;

			dailyLocArray->locData[household_index].location_type = HOUSEHOLD;
			dailyLocArray->locData[household_index].max_contacts = HOUSEHOLD_MAX_CONTACTS;

			for(int j = 0; j < num_adults; j++)
			{
				//build the person and insert them into the vector
				Person * p = &peopleArray[number_people];
				p->id = number_people;
				p->household = household_index;

				//assign age and workplace
				p->workplace = setup_generatePeople_assignAdultWorkplace();
				p->age = 23;  //HACK: age is unused for adults, anything > 22 is treated as equal
				//	printf("\nworkplace %d",p->workplace);
				//	getch();
				number_people++;
			}


			for(int j = 0; j < num_children; j++)
			{
				Person * p = &peopleArray[number_people];
				p->id = number_people;
				p->household = household_index;

				//assign age and workplace
				setup_generatePeople_assignChildAgeAndSchool(p);

				number_people++;
			}
		}
	}
			//	printf("number_peoble %d\n",number_people);

	peopleArray.resize(number_people);

}

void PandemicSim::setup_hoursThatMatter()
{
	if(UNEMPLOYMENT == 1)
	{
		for(int i = HOUR_WORK_START; i <= HOUR_HOME; i++)
			weekday_hours_that_matter.push_back(i);
	}
	else
	{
		weekday_hours_that_matter.push_back(HOUR_WORK_START);
		weekday_hours_that_matter.push_back(HOUR_WORK_END + 1);
		weekday_hours_that_matter.push_back(HOUR_WORK_END + 2);
		weekday_hours_that_matter.push_back(HOUR_HOME);
	}

	for(int i = HOUR_WEEKEND_ERRAND_START; i <= HOUR_HOME; i++)
		weekend_hours_that_matter.push_back(i);
}

void PandemicSim::setup_getUniquePeople(vector<Person *> *victims, int n)
{
	for(int i = 0; i < n; i++)
	{

		Person * v;
		do{
			//get a random person
			v = &peopleArray[myRNG->utility_uniformInt(0,number_people - 1)];
			//ensure they're not a duplicate
			for(int j = 0; j < i; j++){
				if((*victims)[j] == v){
					v = NULL;
					break;}}
		} while(v == NULL); //if he's a duplicate loop back
		(*victims)[i] = v;
	}
}

/*
void PandemicSim::n_people_from_random_workplace(vector<Person *> *victims, int n, int workplace_type)
{
	//build arrays of people's locations
	vector<Person *> workplace_locations(number_workplaces);
	for(int i = 0; i < number_people; i++)
	{
		int wp = peopleArray[i].workplace;
		workplace_locations[wp].push_back(&peopleArray[i]);
	}

	//get the offset to the type of workplace we're searching for
	int wp_offset = 0;
	for(int i = 0; i < workplace_type; i++)
	{
		wp_offset += workplace_data[i][DATA_WORKPLACE_TYPE_COUNT_IDX];
	}

	int type_count = workplace_data[workplace_type][DATA_WORKPLACE_TYPE_COUNT_IDX];
	int start_idx = uniform(0, type_count - 1);
	bool success = false;

	int idx = start_idx;
	do
	{
		int loc_idx = wp_offset + idx;

		int loc_count = workplace_locations[loc_idx].size();
		if(loc_count >= n)
		{
			for(int i = 0; i < n; i++)
			{
				do
				{
					int person_idx = uniform(0, loc_count - 1);
					(*victims)[i] = workplace_locations[loc_idx][person_idx];
					for(int j = 0; j< i; j++)
						if((*victims)[i] == (*victims)[j])
						{
							(*victims)[i] = NULL;
							break;
						}
				} while((*victims)[i] == NULL);
			}
			return;
		}

		idx++;
		if(idx == type_count)
			idx = 0;

	} while(idx != start_idx);

	printf("unable to find enough people for initial infection\n");
	throw;
}*/



void PandemicSim::setup_initialInfection()
{
	//infect generation 0 with pandemic and seasonal viruses

	vector<Person *> victims_pandemic(initial_infected_pandemic);

	//build a vector of initial pandemic victims

	setup_getUniquePeople(&victims_pandemic, initial_infected_pandemic);

	//	int business_type = 4;  //use vals 3-7 for schools, or 2 for workplaces
	//	n_people_from_random_workplace(&victims, initial_infected_pandemic, business_type);

	for(int i = 0; i < initial_infected_pandemic; i++)
	{
		Person * v = victims_pandemic[i];
		//make v sick
		transmitInfection_pandemic(v);

		//all initial pandemic infections are symptomatic
		if(v->profile_pandemic > 3)
		{
			v->profile_pandemic -= 3;
			v->symptomatic_pandemic = true;
		}

		setReproductionNumber(v, ACTION_INFECT_PANDEMIC);
		v->generation_pandemic = 0;
		v->day_pandemic_infection = 0;
	}	

	vector<Person *> victims_seasonal(initial_infected_seasonal);
	setup_getUniquePeople(&victims_seasonal, initial_infected_seasonal);

	//build a vector of initial seasonal infected victims 
	for(int i = 0; i < initial_infected_seasonal; i++)
	{
		Person * v = victims_seasonal[i];

		//make v sick
		transmitInfection_seasonal(v);
		setReproductionNumber(v, ACTION_INFECT_SEASONAL);
		v->generation_seasonal = 0;
		v->day_seasonal_infection = 0;
	}
}

void PandemicSim::setup_scaleSimulation()
{
	number_households = roundHalfUp_toInt(people_scaling_factor * (double) number_households);
	
	int sum = 0;
	for(int business_type = 0; business_type < number_business_types; business_type++)
	{
		//for each type of business, scale by overall simulation scalar
		int original_type_count = roundHalfUp_toInt(workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX]);
		int new_type_count = roundHalfUp_toInt(location_scaling_factor * original_type_count);

		//if at least one business of this type existed in the original data, make sure at least one exists in the new data
		if(new_type_count == 0 && original_type_count > 0)
			new_type_count = 1;

		workplace_data[business_type][DATA_WORKPLACE_TYPE_COUNT_IDX] = new_type_count;
		workplace_type_cumulative_count[business_type] = sum;	//store the exclusive scan value into the cumulative count array

		sum += new_type_count;
	}

	number_workplaces = sum;
	number_locations = number_households + number_workplaces;
	//printf(" \n\n number_workplaces=%d",number_workplaces);
	//	getch();
	//now that we have scaled the simulation, we need to resize the location arrays
	if(weekday_hours_that_matter.size() == 0 || weekend_hours_that_matter.size() == 0)
		throw std::exception();
		//throw std::runtime_error(std::string("Must call setupHoursThatMatter before scaleSimulation"));

	//get the first and last hours of weekday and weekend
	int first_hour = min(weekday_hours_that_matter.front(), weekend_hours_that_matter.front());
	int last_hour = max(weekday_hours_that_matter.back(), weekend_hours_that_matter.back());

	vector<int> hour_range;
	for(int i = first_hour; i <= last_hour; i++)
		hour_range.push_back(i);

	dailyLocArray.resize_selectHours(hour_range,number_workplaces, number_locations);
}

void PandemicSim::setupSimulation()
{
	setup_loadSimParameters();
	setup_hoursThatMatter();

	setup_scaleSimulation();

	//initialize RNG
	srand(seed);
	myRNG = new Pandemic_RNG();

	if(ENABLE_DOCTOR_SAMPLING_EXPERIMENT)
		doctorExperiment = new DoctorExperiment(myRNG);

	fDailyInfectionCounts = fopen("infection_counts.csv", "w");
	fprintf(fDailyInfectionCounts, "day,total,adults,children_5,children_9,children_14,children_17,children_22,work,school,errands,afterschool,home\n");

	// debug outputs
//	infectedWatch = fopen("infected_watch.csv", "w");
//	fprintf(infectedWatch, "day,age,household,workplace,status_pandemic,status_seasonal,generation_pandemic,generation_seasonal,hour_pandemic_infection,hour_seasonal_infected,profile_pandemic,profile_seasonal,reproduction_initial_pandemic,reproduction_initial_seasonal,reproduction_number_pandemic,reproduction_number_seasonal,contacts_made,infected_today\n");

	if(DEBUG_LOG_CONTACTS)
	{
		fContacts = fopen("debug_contacts.csv", "w");
		fprintf(fContacts, "day, person, k_val, status_p, day_p, profile_p, thresh_p, z_p, success_p, status_s, day_s, profile_s, thresh_s, z_s, success_s\n");
	}

	fInfectedStatistics = fopen("output_infected_stats.csv","w");
	fprintf(fInfectedStatistics, "day,number_people,pandemic_susceptible,pandemic_infectious,pandemic_symptomatic,pandemic_asymptomatic,pandemic_recovered,seasonal_susceptible,seasonal_infectious,seasonal_symptomatic,seasonal_asymptomatic,seasonal_recovered\n");

	//generate businesses FIRST
	setup_generateBusinesses(&dailyLocArray);
	setup_generatePeople(&dailyLocArray);  //next generate people and households

	setup_initialInfection();  //last, infect 

	setup_buildStaticLocArrays();
}

#pragma region load
void PandemicSim::setup_loadSimParameters()
{
	FILE * fin;
	fin = fopen("seed.txt", "r");
	fscanf(fin,"%d",&seed);
	fclose(fin);

	fin = fopen("constants.csv","r");
	char * s = (char *) malloc(500 * sizeof(char));
	fgets(s,500,fin);
	fscanf(fin,"%*[^,]%*c");
	fscanf(fin, "%d%*c", &simulation_max_days);
	fscanf(fin, "%f%*c", &reproduction_number_pandemic);
	fscanf(fin, "%f%*c", &reproduction_number_seasonal);
	fscanf(fin, "%d%*c", &initial_infected_pandemic);
	fscanf(fin, "%d%*c", &initial_infected_seasonal);
	fscanf(fin, "%lf%*c", &people_scaling_factor);
	fscanf(fin, "%lf%*c", &location_scaling_factor);
	fscanf(fin, "%f%*c", &percent_symptomatic);
	fscanf(fin, "%f%*c", &asymp_factor);
	fscanf(fin, "%f%*c", &k_hh);
	fscanf(fin, "%f%*c", &k_wp);
	fscanf(fin, "%f%*c", &k_er);
	fscanf(fin, "%d%*c", &ABSENTEEISM);
	fscanf(fin, "%d", &ENABLE_DOCTOR_SAMPLING_EXPERIMENT);
	fclose(fin);

	setup_loadCrossImmunityParameters();

	fin = fopen("households.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= number_household_types;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 3; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&household_data[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("workplaces.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= number_business_types;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 7; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&workplace_data[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("child_ages.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= number_child_age_types;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 3; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&child_age_data[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("adult_ages.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= number_adult_age_types;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 3; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&adult_age_data[i][j]);
		}
	}
	fclose(fin);


	fin = fopen("gamma1.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&gamma1[i][j]);
		//	printf("%f ",gamma1[i][j]);
		}
	//	getch();
	}
	
	fclose(fin);

	fin = fopen("gamma2.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&gamma2[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("lognorm1.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&lognorm1[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("lognorm2.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&lognorm2[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("weib1.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&weib1[i][j]);
		}
	}
	fclose(fin);

	fin = fopen("weib2.csv", "r");
	fgets(s,500,fin);
	for (int i = 0; i <= 10;i++)
	{
		fscanf(fin,"%*[^,]");
		for(int j = 0; j < 2; j++)
		{
			fscanf(fin, "%*c");
			fscanf(fin,"%f",&weib2[i][j]);
		}
	}
	fclose(fin);

	k_values[BUSINESS] = k_wp;
	k_values[SCHOOL] = k_wp;
	k_values[HOUSEHOLD] = k_hh;
	k_values[ERRAND] = k_er;
	k_values[AFTERSCHOOL] = k_er;
}
#pragma endregion load


#pragma region locarray

/*LocationArray * PandemicSim::makeContacts_getLocArray(int weekday, int hour)
{
	
	
	//if absenteeism is on, we need to dynamically generate every hour
	if(ABSENTEEISM == 0)
	{
		//otherwise, we can use pre-generated arrays for household and workplace contact hours
		if(weekend())
		{
			if(hour == HOUR_WEEKEND_HOME)
				return &locations_hh;
		}
		else
		{
			//if enabled, unemployed people will make random errands during workday, so we cannot use pregenerated arrays
			if(hour == HOUR_WORK_START && UNEMPLOYMENT == 0)
				return &locations_wp;

			if(hour == HOUR_WEEKDAY_HOME)
				return &locations_hh;
		}
	}
	

	//else:  use the generated hour
	return &dailyLocArray.locArrays[hour];

}*/


void PandemicSim::deprecated_hourlyScheduler_buildLocArray(LocationArray * currentArray, int hour )
{
	//removes all people from the location array, keeping location data
	currentArray->Empty();

	//move people to their new location	
	for(int i = 0; i < number_people; i++)
	{
		Person * p = &peopleArray[i];

		currentArray->insertPerson(p, p->schedule[hour]);
	}
}

void PandemicSim::setup_buildStaticLocArrays()
{
	//allocate locations
	dailyLocArray.setup_sizeStaticArrays(number_households,number_workplaces);

	for(int i = 0; i < number_people; i++)
	{
		Person * p = &peopleArray[i];
		dailyLocArray.setup_insertPersonStatic(p);
	}
}


#pragma endregion locarray


FILE * fPeople;
FILE * fSchedule;
FILE * fContacts;
FILE * fLocationData;

#pragma region debugAndValidation

void PandemicSim::debug_openStreams()
{
	fPeople = fopen("debug_people_data.csv","w");
	fprintf(fPeople, "day,personId,workplace,household,age,status_pandemic,day_pandemic,generation_pandemic,profile_pandemic,status_seasonal,day_seasonal,generation_seasonal,profile_seasonal,contacts_made,cumulative_k\n");

	fSchedule = fopen("debug_schedules.csv","w");
	fprintf(fSchedule, "day,personId");
	for(int i = 0; i < 24; i++)
	{
		fprintf(fSchedule, ",schedule_Hour%d_Loc,schedule_Hour%d_Type,schedule_Hour%d_ContactsDesired", i, i, i);
	}
	fprintf(fSchedule, "\n");

	fContacts = fopen("debug_contacts.csv","w");
	fprintf(fContacts, "day,personId");
	for(int i = 0; i < 24; i++)
	{
		fprintf(fContacts,",contact_%d_ID,contact_%d_type");
	}
	fprintf(fContacts, "\n");

	fLocationData = fopen("debug_location_data.csv","w");
	fprintf(fLocationData, "day,hour,locIdx,locType,businessType,maxContacts,people\n");
}

void PandemicSim::debug_dumpPeople()
{
	for(int i = 0; i < number_people; i++)
	{
		Person * p = &peopleArray[i];
		fprintf(fPeople, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%lf\n",
			sim_day, i,p->workplace,p->household,p->age,
			p->status_pandemic,p->day_pandemic_infection,p->generation_pandemic,p->profile_pandemic,
			p->status_seasonal,p->day_seasonal_infection,p->generation_seasonal,p->profile_seasonal,
			p->contacts_made,p->cumulative_k);
	}
	fflush(fPeople);
}

void PandemicSim::debug_dumpContacts()
{
	for(int i = 0; i < infected_array.size(); i++)
	{	
		Person * p = infected_array[i];
		fprintf(fContacts,"%d,%d",sim_day,i);
		for(int i = 0; i < p->contacts_made; i++)
		{
			fprintf(fContacts, ",%d,%d",
				p->contact_ids[i],p->contact_type[i]);
		}
		fprintf(fContacts, "\n");
	}
	fflush(fContacts);
}

void PandemicSim::debug_dumpLocations( LocationArray * locArray )
{
	for(int i = 0; i < number_locations; i++)
	{
		Location * l = &locArray->locs[i];
//		if(l->count_hourly_total > 0)
		{
			fprintf(fLocationData,"%d,%d,%d,%d,%d,",
				sim_day, sim_hour, 
				dailyLocArray.locData[i].location_type,
				dailyLocArray.locData[i].business_type, 
				dailyLocArray.locData[i].max_contacts);
			for(int j = 0; i < l->count_hourly_total; i++)
				fprintf(fLocationData, "%d ",l->get_person(j)->id);
			fprintf(fLocationData, "\n");
		}

	}
	fflush(fLocationData);
}

void PandemicSim::schedule_countPersonStatus(int personIdx)
{
	Person * p = &peopleArray[personIdx];

	int status_p = p->status_pandemic;
	count_pandemic_status[status_p]++;

	int status_s = p->status_seasonal;
	count_seasonal_status[status_s]++;
}

void PandemicSim::output_dailyInfectedStatusCount()
{
	fprintf(fInfectedStatistics,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		sim_day,number_people,
		count_pandemic_status[0], //suscep
		count_pandemic_status[1] + count_pandemic_status[2],	//inf
		count_pandemic_status[1],	//symp
		count_pandemic_status[2], //asymp
		count_pandemic_status[3],	//recovered
		count_seasonal_status[0], //suscep
		count_seasonal_status[1] + count_seasonal_status[2], //inf
		count_seasonal_status[1],  //symp
		count_seasonal_status[2], //asymp
		count_seasonal_status[3]); //recovered

}


void PandemicSim::setup_loadCrossImmunityParameters()
{
	
	/*
	fscanf(fin, "%f%*c", &epsilon_p);
	fscanf(fin, "%f%*c", &epsilon_pr);
	fscanf(fin, "%f%*c", &epsilon_ps);
	fscanf(fin, "%f%*c", &epsilon_s);
	fscanf(fin, "%f%*c", &epsilon_sr);
	fscanf(fin, "%f%*c", &epsilon_sp);*/

	//cross immunity factor experiment
	epsilon_p = 1.f;
	epsilon_pr = 1.f;
	epsilon_ps = 1.f;
	epsilon_s = 1.f;
	epsilon_sr = 1.f;
	epsilon_sp = 1.f;
}

void PandemicSim::output_dailyTransmissions()
{
	fprintf(fDailyInfectionCounts, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		sim_day,
		today_infectedBoth + today_infectedPandemic + today_infectedSeasonal,
		today_infected_adults,
		today_infected_childrenAge5,
		today_infected_childrenAge9,
		today_infected_childrenAge14,
		today_infected_childrenAge17,
		today_infected_childrenAge22,
		today_infectedWork,
		today_infectedSchool,
		today_infectedErrands,
		today_infectedAfterschool,
		today_infectedHome);
	fflush(fDailyInfectionCounts);

	today_infectedBoth = 0;
	today_infectedPandemic = 0;
	today_infectedSeasonal = 0;

	today_infectedAfterschool = 0;
	today_infectedErrands = 0;
	today_infectedHome = 0;
	today_infectedSchool = 0;
	today_infectedWork = 0;

	today_infected_childrenAge5 = 0;
	today_infected_childrenAge9 = 0;
	today_infected_childrenAge14 = 0;
	today_infected_childrenAge17 = 0;
	today_infected_childrenAge22 = 0;
	today_infected_adults = 0;
}

#pragma endregion debugAndValidation

#pragma region staticfunctions

int roundHalfUp_toInt(double d)
{
	return floor(d + 0.5);
}

void count_age_and_location(int age, int loc_type)
{
	switch(age)
	{
	case 5:
		today_infected_childrenAge5++;
		break;
	case 9:
		today_infected_childrenAge9++;
		break;
	case 14:
		today_infected_childrenAge14++;
		break;
	case 17:
		today_infected_childrenAge17++;
		break;
	case 22:
		today_infected_childrenAge22++;
		break;
	case 23:
		today_infected_adults++;
		break;
	default:
		printf("unknown age, val: %d\n", age);
		throw;
	}

	switch(loc_type)
	{
	case BUSINESS:
		today_infectedWork++;
		break;
	case SCHOOL:
		today_infectedSchool++;
		break;
	case HOUSEHOLD:
		today_infectedHome++;
		break;
	case ERRAND:
		today_infectedErrands++;
		break;
	case AFTERSCHOOL:
		today_infectedAfterschool++;
		break;
	default:
		printf("unknown loc type, val %d\n", loc_type);
		throw;
	}
}
#pragma endregion staticfunctions


