#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define culmination_period 240 // after these many hrs the person dies or recover
#define business_capacity 4000 // max business capacity (# of people)
#define household_capacity 10 // max household capacity (# of people)
#define contact_capacity 50 // max number of contacts per person per day (# of people)
#define number_business_type 14 // # of business types (including home)
#define number_adult_age_type 3 // # of age types for adults
#define number_child_age_type 5 // # of age types for kids
#define number_household 40000 // # of households in one population center
#define number_household_type 9 // # of household types
#define number_population_center 1 //# of population centers in the community
#define number_zipcodes 57 //# of population centers in the community
float asymp = 0;// factor by which the infectiousness profile of an asymptomatic is reduced. asymp = 4.4954/20.2094.  Cite: Carrat08
float epsilon_p=0;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile that is also occuring(e.g. cross-immunity created by the seasonal virus)
float epsilon_pr=0;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile that has already passed(e.g. cross-immunity created by the seasonal virus)
float epsilon_ps=0;//factor by which a pandemic infectious profile is affected given the influence of a seasonal profile when both are acurring at the same time(e.g. cross-immunity created by the seasonal virus)
float epsilon_s=0;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile that is also occuring(e.g. cross-immunity created by the pandemic virus)
float epsilon_sr=0;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile that has already passed(e.g. cross-immunity created by the pandemic virus)
float epsilon_sp=0;//factor by which a seasonal infectious profile is affected given the influence of a pandemic profile when both are acurring at the same time(e.g. cross-immunity created by the pandemic virus)
float k_hh=0;// proportion of duration and closeness of a household contact with respect to the household contact  k_hh=1. Cite: Mossong06
float k_wp=0;// proportion of duration and closeness of a workplace contact with respect to the household contact k_wp=0.67.  Cite: Mossong06
float k_er=0;// proportion of duration and closeness of an errand contact with respect to the household contact k_er=0.44. Cite: Mossong06
int initial_infected_pandemic=0;
int initial_infected_seasonal=0;
int max_days=0;// number of simulation days
float reproduction_number_pandemic=0.0;
float reproduction_number_seasonal=0.0;
long seed = 0;


// Declaring business structures and their attributes
struct entity
{  
    int	  age; // age of a person
	int	  contact; // 5 = yes, 0 = no
	unsigned long   contact_array_id[contact_capacity][1+2]; // array of people ids in any given hr
	//int	  contactor_id;  // id track.
	int   day_begin_infection_pandemic; // day of the outbreak the infection begins
	int   day_begin_infection_seasonal; // day of the outbreak the infection begins
	float	  disease_clock_pandemic; //0 = alive and well, -1 = dead, -3 = recover;
	float	  disease_clock_seasonal; //0 = alive and well, -1 = dead, -3 = recover;
	int   contact_counter; //To accumulate the number of contacts per day
	int   contact_counter_hh;//To accumulate the number of contacts in the household per day
    	int   contact_counter_wp;//To accumulate the number of contacts in the workplace/school per day
	int   contact_counter_er;//To accumulate the number of contacts in the errands per day
	int   generation_pandemic;
    	int   generation_seasonal;
	long  household_number; // # of the household the person belongs to
	unsigned  long  household_member; // # of household members
	unsigned long  id; // unique # for each person in the community
	int	  infection_day_pandemic; // >0 = number of days since infection began; -6 = alive and well; -2 = infected; -5 = recovered
   	int	  infection_day_seasonal; // >0 = number of days since infection began; -6 = alive and well; -2 = infected; -5 = recovered
	int	  profile_pandemic; // 0 = none, 1 = Gamma1, 2 = Lognormal1, 3 = Weibull1, 4 = Gamma2, 5 = Lognormal2, 6 = Weibull2
	int	  profile_seasonal; // 0 = none, 1 = Gamma1, 2 = Lognormal1, 3 = Weibull1, 4 = Gamma2, 5 = Lognormal2, 6 = Weibull2
	float rn_init_pandemic; // initial reproduction number
    	float rn_init_seasonal; // initial reproduction number
	int	  rn_pandemic; // final reproduction number pandemic
    	int	  rn_seasonal; // final reproduction number pandemic
	int	  schedule[25]; // daily schedule: hour & business id; 1 = home; -1 = dead; -2 = travel;
	int   errand[3+1][3+1]; //To record the contact rate of the unemployed
    	int   errandw[3+1][3+1]; //To record the contact rate on weekends
	int   symptomatic; // 0 = non-infected; 1 = symptomatic; 2 = asymptomatic;
	int   virus; // 0 = none; 1 = pandemic; 2 = seasonal; 3 = both; keeps track of the set of strains that the individual is currently using to infect
	int   prior_virus; // 0 = none; 1 = pandemic; 2 = seasonal; 3 = both; keeps track of the set of strains that have infected the individual so far
	unsigned long  workplace; // ID of a person's (permanent) workplace in the community
	int   zipcode; //zipcode number
	
};

// Declaring households structures and their attributes
struct house
{
	unsigned long    array_id[household_capacity][1+1]; // array of people ids in any given hr
 	int    count_infected;  // total # of infected in any given hr
	unsigned int    count_total; // total # of people in any given hr
};

struct bus
{
	unsigned long		array_id[business_capacity][2+1]; // array of people ids in any given hr
	float	contact_rate; // contact rate (per one infected person)
	int		count_infected; // total # of infected in any given hr
	int		count_total_2[24]; // total # of people in any given hr.  To check the assignment of business to people
	int		count_total; // total # of people in any given hr.  To check the assignment of people to business
	unsigned long		id; // unique ID number
	int		type; 
	float		vol_quarantine; // cum % of voluntary quarantine
	float		weekday; // cum % of after work errands Mon-Fri
	float		weekend; // cum % of errands Sat-Sun
	long double	work; // cum % of workplaces for this business entity
};

// Declaring functions
//void  assign_zipcode(void);
int	  assign_workplace(void);
int   assign_school(int age1);
void  check();
int contact_business (unsigned long entity, int entity_location_hour);
int contact_household (unsigned long entity, int entity_location_hour);
void  declare_location(int hour);//locate people in their workplaces or households according to the initial place assignment
void  disease_progress(int hour);
void  disease_spread(int hour,int day);
void  generate_businesses(void); // generate workplaces
void  generate_entities(void);  // generate households and people
void  infection (unsigned long id, float k, int day, unsigned long i, int contact_rate_hh, int contact_rate_wp, int contact_rate_er);
float infection_probability (float repnum, int profile, int infection_day, int contact_rate_hh, int contact_rate_wp, int contact_rate_er);
void  profile_assignment_pandemic(unsigned long i);
void  profile_assignment_seasonal(unsigned long i);
float random1(void); // generate a random number between 0 and 1
void read_constants(void);
void  read_input(void);  // read input data files
void  recover_process_pandemic(unsigned long id);
void  recover_process_seasonal(unsigned long i);
void  schedule_adult_weekday(int id1); // generate an adult's daily schedule for Mon-Fri
void  schedule_child_weekday(int id2); // generate a child's daily schedule for Mon-Fri
void  schedule_regular(int day); // assign a regular day schedule
void  schedule_weekend(int id3); // generate a family's daily schedule for Sat-Sun
float uni(unsigned long a,unsigned long b); // generate a random number between a & b

// Declaring global variables/arrays
//struct	entity community[number_population_center*number_household+1];//CAUTION!!! It should have more entities than the amount of people
//struct  house household[(number_population_center*number_household)+1];//CAUTION!!! It should have more spaces than the amount of households
struct	house *household;
struct	entity *community;
//struct	bus business[128010];//CAUTION!!! It should have more spaces than the amount of businesses
struct 	bus *business;//=malloc(128001*sizeof(struct bus));
int		number_businesses = 0; // total number of businesses in the community
float   percent_symptomatic = 0.669; // 66.9% of people present symptomatic infection Carrat08
int		t_now = 0; // simulation clock time
int 	total_infected_pandemic = 0;
int 	total_infected_seasonal = 0;
int     total_sim_coinfected = 0;
int 	total_infected_pandemic_only = 0;
int 	total_infected_seasonal_only = 0;
int 	total_coinfected_s_p = 0;
int 	total_reinfected_s_p = 0;
int 	total_coinfected_p_s = 0;
int 	total_reinfected_p_s = 0;
int		uniform_counter = 1;

unsigned long	N = 0; // total number of entities in the community
int	    new_infected_today_pandemic = 0; // number of new infected entities every day
int	    new_infected_today_seasonal = 0; // number of new infected entities every day
int	    new_infected_today_pandemic_only = 0; // number of new infected entities every day
int	    new_infected_today_seasonal_only = 0; // number of new infected entities every day
int	    new_sim_coinfected = 0; // number of new infected entities every day
int	    new_coinfected_s_p = 0; // number of new infected entities every day
int	    new_reinfected_s_p = 0; // number of new infected entities every day
int	    new_coinfected_p_s = 0; // number of new infected entities every day
int	    new_reinfected_p_s = 0; // number of new infected entities every day
int	    new_recovered_today_pandemic = 0; // number of new recovered entities every day
int	    new_recovered_today_seasonal = 0; // number of new recovered entities every day

float	age_adult[10][2+1];
float	age_child[10][3+1];
float	households[20][3+1];
float	uniform[10001]; // an array of random numbers
float   workplaces[20][7+1];
float   zipcode[58][2+1];
float   gamma1[11][2]; //gamma CDF profile of the symptomatic individuals
float   gamma2[11][2]; //gamma CDF profile of the asymptomatic individuals
float   lognorm1[11][2]; //lognormal CDF profile of the symptomatic individuals
float   lognorm2[11][2]; //lognormal CDF profile of the asymptomatic individuals
float   weib1[11][2]; //weibul CDF profile of the symptomatic individuals
float   weib2[11][2]; //weibul CDF profile of the asymptomatic individuals

FILE *input1, *input2, *input3, *input4, *input5, *input8, *input9, *input10, *input11, *input12, *input13, *input_constants, *output1, *output2, *output3, *output4;

int main()
{
	business=malloc(128001*sizeof(struct bus));
	community=malloc(3*number_household*number_population_center*sizeof(struct entity));
	household=malloc(10+number_household*number_population_center*sizeof(struct house));
    int i,a,d,hr;
	float w;
	//int i,j;
	int b,k;
	unsigned int total_household, total_business; 
	unsigned int total_schedule;
	double time1=clock();
	read_constants();
    read_input();

//printf("read file:%d\n",time1);
    generate_businesses();
//time1=clock()-time1;
//printf("gen-business%d\n",time1);
    generate_entities();
//time1=clock()-time1;
//printf("gen-entities%d\n",time1);

   // assign_zipcode();

 	// I am not including the initialize routine because I don't need it yet

	// Opening output3 file
	output3 = fopen("output_general_stats.txt","w");
	fprintf(output3, "Day  Population  Infected_p  Recovered_p  Infected_s  Recovered_s  Infected_p_only  Infected_s_only coinfected_s_p reinfected_s_p coinfected_p_s reinfected_p_s\n coinfected sim");

	 //bring an infected person of pandemic influenza

	for (i = 1; i <= initial_infected_pandemic; ++i){
		a = (int)uni(1,N);
		
		community[a].disease_clock_pandemic = 0;
		community[a].generation_pandemic=1;
		community[a].infection_day_pandemic = -2;
		community[a].virus = 1;
		community[a].prior_virus = 1;
		community[a].rn_init_pandemic = reproduction_number_pandemic;
		printf("%d person with pandemic virus is %d with disease clock %d \n", i, a, community[a].disease_clock_pandemic);
	}
	
    for (i = 1; i <= initial_infected_seasonal; ++i){
		a = (int)uni(1,N);
		community[a].disease_clock_seasonal = 0;
		community[a].generation_seasonal = 1;
		community[a].infection_day_seasonal = -2;
		community[a].virus = 2;
		community[a].prior_virus = 2;
		community[a].rn_init_seasonal = reproduction_number_seasonal;
		printf("%d person with seasonal virus is %d with disease clock %d \n", i, a, community[a].disease_clock_seasonal);
	}

//	getchar();

	while(t_now < 10*24) { // daily simulation begins ... max_days
		
		d = 1+t_now/24; // A day begins ...
		if (d >=2){
			hr=(t_now-(24*(d-1)))+1;
		}

		else {
			hr=t_now+1;            
		}
		fprintf(output1,"\nDay %d  Hour %d\n\n", d,hr);
		printf("\nDay %d  Hour %d\n\n", d,hr);

		new_sim_coinfected = 0;
		new_infected_today_pandemic = 0;
		new_infected_today_pandemic_only = 0;
		new_coinfected_s_p = 0;
		new_reinfected_s_p = 0;
		new_recovered_today_pandemic = 0;
		new_infected_today_seasonal = 0;
		new_infected_today_seasonal_only = 0;
		new_coinfected_p_s = 0;
		new_reinfected_p_s = 0;
		new_recovered_today_seasonal = 0;


		//assign schedules to everyone at the beginning of the day


		if (hr==1){
		schedule_regular(d);
		//total_schedule=0;
		}
		
	    // check the amount of people per hour per day
		//for (i = 1; i <= number_businesses; ++i) {
				
				//business[i].count_total_2[hr]=0;

				//for (j = 1; j <= N; ++j) {
                    //fprintf(output1, "Person %d with age %d is in the business %d in the hour %d and day %d \n",j,community[j].age,community[j].schedule[hr],hr,d);
                    //if (community[j].schedule[hr]==i ){
					   //++business[i].count_total_2[hr];                    
				    //}
				//}
			    //total_schedule = total_schedule + business[i].count_total_2[hr];
                //fprintf(output1, "There are %d people in the business %d in the hour %d and day %d \n",business[i].count_total_2[hr],i,hr,d);
		//}
        //fprintf(output1, "There are %d people in the hour %d and day %d \n",total_schedule,hr,d);

		disease_progress(hr);
		declare_location(hr);
		disease_spread(hr,d);

		total_household = 0;
		total_business = 0;
        
		// check the amount of people per business per hour per day

		for (b=1; b<=(number_population_center*number_household); ++b){
			total_household = total_household + household[b].count_total;
		}
		fprintf(output1, "There are %d household people in the hour %d and day %d \n",total_household,hr,d);

        for (k = 2; k <= number_businesses; ++k) {
		    total_business = total_business + business[k].count_total;	
		}
        fprintf(output1, "There are %d business people in the hour %d and day %d \n",total_business,hr,d);

		if (hr==24){
			fprintf(output3,"%d %d %d %d %d %d %d %d %d %d %d %d %d \n",d,total_business+total_household,total_infected_pandemic,new_recovered_today_pandemic,total_infected_seasonal,new_recovered_today_seasonal,total_infected_pandemic_only,total_infected_seasonal_only,total_coinfected_s_p,total_reinfected_s_p,total_coinfected_p_s,total_reinfected_p_s,total_sim_coinfected);
		}

		++t_now;
	}
    check();//Calculate the average reproduction number
//	getch();
 double time2=clock();
time1=time2-time1;
time1=time1/1000000;
printf("elapsed time for %llu Agents are %f",N,time1);
return 0;
}
// **************************************************************************	
void read_constants(void) // read simulation's contants
{
	input_constants = fopen("constants.txt","r");
	
	fscanf(input_constants, "%d", &seed);
	fscanf(input_constants, "%d", &max_days);
	fscanf(input_constants, "%f", &reproduction_number_pandemic);
    fscanf(input_constants, "%f", &reproduction_number_seasonal);
	fscanf(input_constants, "%d", &initial_infected_pandemic);
    fscanf(input_constants, "%d", &initial_infected_seasonal);
	fscanf(input_constants, "%f", &asymp);
	fscanf(input_constants, "%f", &k_hh);
	fscanf(input_constants, "%f", &k_wp);
	fscanf(input_constants, "%f", &k_er);
	fscanf(input_constants, "%f", &epsilon_p);
	fscanf(input_constants, "%f", &epsilon_pr);
	fscanf(input_constants, "%f", &epsilon_ps);
	fscanf(input_constants, "%f", &epsilon_s);
	fscanf(input_constants, "%f", &epsilon_sr);
	fscanf(input_constants, "%f", &epsilon_sp);

	fclose(input_constants);
}

// **************************************************************************	
void read_input()
{
	int i, j;
	input1 = fopen("households.txt","r");
	input2 = fopen("workplaces.txt","r");
	input3 = fopen("age_child.txt","r"); 
	input4 = fopen("age_adult.txt","r");
	input5 = fopen("zipcode.txt","r");
	input8 = fopen("gamma1.txt","r");
	input9 = fopen("gamma2.txt","r"); 
    input10 = fopen("lognorm1.txt","r");
	input11 = fopen("lognorm2.txt","r");
    input12 = fopen("weib1.txt","r");
	input13 = fopen("weib2.txt","r");
	output1 = fopen("output.txt","w");
	output2 = fopen("output_table_1.txt","w");
	fprintf(output2, "Day Hour Contactor_id  Cont_rate  Contactor_inf_day  Contacted_id  Contacted_inf_day  Contacted_gen  Contactor_id  Contactor_rn Contactor_symptomatic Contactor_profile\n");

	for (i = 1; i <= 10000; ++i) 
	{
		uniform[i] = random1();
	}

	for (i = 0; i <= number_household_type; ++i)
	{
		for (j = 1; j <= 3; ++j)
		{
			fscanf(input1, "%f", &households[i][j]);
		}
	}
	
	for (i = 0; i <= number_business_type; ++i)
	{
		for (j = 1; j <= 7; ++j)
		{
			fscanf(input2, "%f", &workplaces[i][j]);
		}
//	    printf("The number of type %d businesses is %g\n",i,workplaces[i][2]);
	}

	for (i = 0; i <= number_child_age_type; ++i)
	{
		for (j = 1; j <= 3; ++j)
		{
			fscanf(input3, "%f", &age_child[i][j]);
		}
//		printf("Type %d child has age %g and a school %g\n",i,age_child[i][1],age_child[i][3] );

	}

	for (i = 0; i <= number_adult_age_type; ++i)
	{
		for (j = 1; j <= 2; ++j)
		{
			fscanf(input4, "%f", &age_adult[i][j]);
		}
	}

	for (i = 0; i <= number_zipcodes; ++i)
	{
		for (j = 1; j <= 2; ++j)
		{
			fscanf(input5, "%f", &zipcode[i][j]);
		}
		//printf("zipcode %g has a proportion %g \n",zipcode[i][1],zipcode[i][2]);
	}

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input8, "%f", &gamma1[i][j]);
		}
	}

	//printf("the gamma 1 cdf is %f \n", gamma1[10+1][1]);

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input9, "%f", &gamma2[i][j]);
		}
	}
	//printf("the gamma 2 cdf is %f \n", gamma2[10+1][1]);

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input10, "%f", &lognorm1[i][j]);
		}
	}
//	printf("the lognormal 1 cdf is %f \n", lognorm1[10+1][2]);

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input11, "%f", &lognorm2[i][j]);
		}
	}
	//printf("the lognormal 2 cdf is %f \n", lognorm2[10+1][2]);

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input12, "%f", &weib1[i][j]);
		}
	}
	//printf("the weibull 1 cdf is %f \n", weib1[1+1][2]);

	for (i = 0; i <= 10; ++i)
	{
		for (j = 0; j <= 1; ++j)
		{
			fscanf(input13, "%f", &weib2[i][j]);
		}
	}
	//printf("the weibull 2 cdf is %f \n", weib2[10+1][2]);
}

// ***************************************************************************
void generate_businesses(void) // generate all business entities
{
	int i, k, s=0;
	for (i = 1; i <= number_business_type; ++i) {
		for (k = s+1; k <= s+workplaces[i][2]; ++k) {
			business[k].id = k;
			business[k].type = i-1;
			business[k].work = business[k-1].work+(long double)workplaces[i][3]/workplaces[i][2];
			business[k].weekday = business[k-1].weekday+(float)workplaces[i][4]/workplaces[i][2];
			business[k].weekend = business[k-1].weekend+(float)workplaces[i][5]/workplaces[i][2];
			business[k].vol_quarantine = business[k-1].vol_quarantine+(float)workplaces[i][6]/workplaces[i][2];
			business[k].contact_rate = workplaces[i][7];
			++number_businesses;
		}
		s+=workplaces[i][2];
	}
	printf("Total number of business is %d\n",number_businesses);
	fprintf(output1, "Total number of business is %d\n",number_businesses);
}

// **************************************************************************	
void generate_entities(void)
{
	int i,s,m,n1,n2,j,p; 
	unsigned long k;
	float y,z;
	long counter=0;


	for (i=1;i<=number_population_center;++i) {
		for (k=1;k<=number_household;++k) {

			y = uni(0,1); // n1 = #adults, n2 = # kids
			for (s=1;s<=number_household_type;++s) {
				if (y>=households[s-1][3] && y < households[s][3]) {
					n1=households[s][1];
					n2=households[s][2];
					
				}
			}
			counter++;
		if(N%500000==0)
		printf("%d\n",N);
			// Start generating an adult entity *************************
			for (m=1;m<=n1;++m) {
				++N;
				community[N].contact = 0;
				community[N].contact_counter = 0;
				community[N].contact_counter_hh = 0;
                community[N].contact_counter_wp = 0;
                community[N].contact_counter_er = 0;
				//community[N].contactor_id = 0;
				community[N].day_begin_infection_pandemic = 0;
                community[N].day_begin_infection_seasonal = 0;
                community[N].disease_clock_pandemic  = 0;
                community[N].disease_clock_seasonal  = 0;
				community[N].errand[1][1] = 0; 
                community[N].errand[2][1] = 0; 
                community[N].errand[3][1] = 0; 
				community[N].errand[1][2] = 0;
                community[N].errand[2][2] = 0;
                community[N].errand[3][2] = 0;
                community[N].errand[1][3] = 0;
                community[N].errand[2][3] = 0;
                community[N].errand[3][3] = 0;
				community[N].errandw[1][1] = 0; 
                community[N].errandw[2][1] = 0; 
                community[N].errandw[3][1] = 0; 
				community[N].errandw[1][2] = 0;
                community[N].errandw[2][2] = 0;
                community[N].errandw[3][2] = 0;
                community[N].errandw[1][3] = 0;
                community[N].errandw[2][3] = 0;
                community[N].errandw[3][3] = 0;
                community[N].generation_pandemic = 0;
                community[N].generation_seasonal = 0;
				community[N].id=N;
				community[N].household_number=(i-1)*number_household+k;
				community[N].household_member=n1+n2;
				community[N].infection_day_pandemic = -6;
                community[N].infection_day_seasonal = -6;
				community[N].prior_virus = 0;
				community[N].profile_pandemic = 0;
                community[N].profile_seasonal = 0;
				community[N].symptomatic = 0;
				community[N].rn_init_pandemic = 0;
                community[N].rn_init_seasonal = 0;
				community[N].rn_pandemic = 0.0;
                community[N].rn_seasonal = 0.0;
				community[N].virus = 0;
                community[N].zipcode = 0;

				for (p=1; p<=contact_capacity; p++){
					community[N].contact_array_id[p][1]=0;
					community[N].contact_array_id[p][2]=0;
				}

				community[N].workplace = assign_workplace();

				// Assign Age
				if (y <= age_adult[1][2]) {
					community[N].age = (int)uni(22,age_adult[1][1]);
				}
				if (y >= age_adult [1][2] && y <= age_adult[2][2]) {
					community[N].age = (int)uni(age_adult[1][1],age_adult[2][1]);
				}
				else {
					community[N].age = (int)uni(age_adult[2][1],age_adult[3][1]);
				}

				if( community[N].workplace == 0){
					printf("Problem adult, N %d, Age %d, Workplace %d\n", N, community[N].age, community[N].workplace);
				}

			} // End generating an adult entity **************************
			
			// Start generating a child entity ****************************
			for (m=1;m<=n2;++m) {
				++N;
				community[N].contact = 0;
				community[N].contact_counter = 0;
				community[N].contact_counter_hh = 0;
                community[N].contact_counter_wp = 0;
                community[N].contact_counter_er = 0;
				//community[N].contactor_id = 0;
				community[N].day_begin_infection_pandemic = 0;
                community[N].day_begin_infection_seasonal = 0;
				community[N].disease_clock_pandemic  = 0;
				community[N].errandw[1][1] = 0; 
                community[N].errandw[2][1] = 0; 
                community[N].errandw[3][1] = 0; 
				community[N].errandw[1][2] = 0;
                community[N].errandw[2][2] = 0;
                community[N].errandw[3][2] = 0;
                community[N].errandw[1][3] = 0;
                community[N].errandw[2][3] = 0;
                community[N].errandw[3][3] = 0;
                community[N].generation_pandemic = 0;
                community[N].generation_seasonal = 0;
				community[N].id=N;
				community[N].household_number=(i-1)*number_household+k;
				community[N].household_member=n1+n2;
				community[N].infection_day_pandemic = -6;
                community[N].infection_day_seasonal = -6;
				community[N].prior_virus = 0;
				community[N].profile_pandemic = 0;
                community[N].profile_seasonal = 0;
				community[N].symptomatic = 0;
				community[N].rn_init_pandemic = 0;
                community[N].rn_init_seasonal = 0;
				community[N].rn_pandemic = 0.0;
                community[N].rn_seasonal = 0.0;
				community[N].virus = 0;
				community[N].zipcode = 0;

				for (p=1; p<=contact_capacity; p++){
					community[N].contact_array_id[p][1]=0;
					community[N].contact_array_id[p][2]=0;
				}

				// Assign Age	
				z = uni(0,1);
				for (j = 1; j <= number_child_age_type; ++j) {
					if (z >= age_child[j-1][2] && z < age_child[j][2]) {
						community[N].age = age_child[j][1];
					}
				}

				community[N].workplace = assign_school(community[N].age);
				if( community[N].workplace == 0 || community[N].workplace > number_businesses){
					printf("Problem Child, N %d, Age %d, Workplace %d\n", N, community[N].age, community[N].workplace);
				}

			} // End generating a child entity*****************************
		}
       


	}
    printf("Total number of people is %d\n  counter:%d", N,counter);
	fprintf(output1, "Total number of people is %d\n", N);
	

}
// **************************************************************************
/*void assign_zipcode()
{
	int i, total_per_zipcode, total_all_zipcode, subtotal_per_zipcode, j, k, STEP, prev_total_per_zipcode, discount,z;
	total_all_zipcode=0;
    prev_total_per_zipcode=1;
	for (i = 1; i <= number_zipcodes; ++i) {
		total_per_zipcode=0;
        subtotal_per_zipcode=0;
		total_per_zipcode=zipcode[i][2]*N;
		total_all_zipcode=total_all_zipcode + total_per_zipcode;
		STEP=0;
		discount=0;
		//To adjust for the fractions of people that are not included in the integer total_per_zipcode.  The remaining population is included in the last zipcode
		if (i==number_zipcodes){
			total_per_zipcode=total_per_zipcode+(N-total_all_zipcode);
		}
		//fprintf(output1,"Total number of people per zipcode is %d\n", total_per_zipcode);
		for (j = prev_total_per_zipcode; j <= (prev_total_per_zipcode + total_per_zipcode); j = j+STEP){
			subtotal_per_zipcode=subtotal_per_zipcode+community[j].household_member;
			if (subtotal_per_zipcode<=total_per_zipcode ){
				for (k = j; k <= (j + community[j].household_member-1); ++k){
					community[k].zipcode=zipcode[i][1];
                    //fprintf(output1,"person %d is in the hh %d with %d hh members and zipcode %d and subtotal is %d \n", k, community[k].household_number,community[k].household_member, community[k].zipcode, subtotal_per_zipcode);
				}
			    STEP = community[j].household_member;
				discount = 0;
			}
			else{
				STEP = total_per_zipcode;
				discount = community[j].household_member;
			}
		}
    prev_total_per_zipcode=prev_total_per_zipcode+subtotal_per_zipcode-discount;//remember: in principle it should be prev_total_per_zipcode-1(prev..begins by 1 and not by 0)+subtotal_per_zipcode-discount+1(to avoid considering the last element of the previous zipcode)
	}
	//fprintf(output1,"the count of people in all zipcodes is %d \n", prev_total_per_zipcode);
	if (i>number_zipcodes){
			for(z=prev_total_per_zipcode; z<=N; z++){
				community[z].zipcode=zipcode[number_zipcodes][1];
				//fprintf(output1,"person %d is in the hh %d with %d hh members and zipcode %d \n", z, community[z].household_number,community[z].household_member, community[z].zipcode);
			}
	}
	//getchar();
}*/
// **************************************************************************	
int assign_workplace(void)
{
	float y;
	int i;
	int aux;
	int low;
	int high;
	int mid;

    y = uni(0,1);
	low = 1; 
	high = number_businesses;
	mid = (low + high)/2;
	while ( high - low >= 5) {
		if( y <= business[mid].work ){
			high = mid;
			mid = (low + high)/2;
		}
		if( y > business[mid].work ){
			low = mid;
			mid = (low + high)/2;
			}
	}
	
	for (i = low; i <= high; ++i) {
		if (y >= business[i-1].work && y < business[i].work) {
			aux= business[i].id;
		}
	}
	return aux;
}
// **************************************************************************	
int assign_school(int age1)
{
	int i, t, z, aux = 0;

	// fish out the type of school based on the age
	for (i = 1; i <= number_child_age_type; ++i) {
		if (age_child[i][1] == age1) t = age_child[i][3];
	}

	z = (int)uni(0,workplaces[t+1][2]); //for the workplace array.  In notepad t = age_child[i][3] is correct but in C t+1 = age_child[i][3] gives the correct position for the school type in the workplace array.  Print workplace [i][2] to understand
	
	for (i = 1; i <= number_businesses; ++i) {
		if (business[i].type == t) {
			//aux = business[i+z].id;
			aux = i+z - 1;
			i = number_businesses + 1;
		}
	}

	return aux;
}

// **************************************************************************	
void schedule_regular(int day)
{
	unsigned long i,j;
	if (day%7 != 0 && day%7 != 6) { //a weekday begins ...
		for (i = 1; i <= N; ++i) { // assign every person his regular weekday schedule:
			//if (community[i].disease_clock != -1 && community[i].schedule[1] >= 0) {
			if(community[i].schedule[1] >= 0){
			   if (community[i].age > 22) schedule_adult_weekday(i);
			   if (community[i].age <= 22) schedule_child_weekday(i);
			}
		}
	}
	else { //a weekend day begins ...
		for (j = 1; j <= N; ++j) { // assign every person his regular weekend schedule:
			//if (community[j].disease_clock != -1 && community[j].schedule[1] >= 0) {
			if (community[j].schedule[1] >= 0) {
				schedule_weekend(j);
			}
		}
	}

}

// **************************************************************************	
void schedule_adult_weekday(int id1)
{
	int i,j,k,l;
	int low, mid, high;
	int temp[4];
    int i1,i2,i3,prov;
	int r;
	int c[3+1];
	float y;

	// initialize schedule to 'home = 1' for 24 hrs
	for (i = 1; i <= 24; ++i) {
		community[id1].schedule[i] = 1;
	}

	// check if the person works
	if (community[id1].workplace != 1) {
		// send the person to his work from 8 to 17
		for (j = 8; j <= 17; ++j) {
			community[id1].schedule[j] = community[id1].workplace;
		}
		// send the person to afterwork errands j = 1, 2 after 17
		for (k = 1; k <= 2; ++k) {
			y = uni(0, 1);

			low = 1; 
			high = number_businesses;
			mid = (low + high)/2;
			while ( high - low >= 5) {
				if( y <= business[mid].weekday ){
					high = mid;
					mid = (low + high)/2;
				}
				if( y > business[mid].weekday ){
					low = mid;
					mid = (low + high)/2;
				}
			}
			// fish out y in the cum % of weekday errands in business entities
			for (l = low; l <= high; ++l) {
				if (y >= business[l-1].weekday && y < business[l].weekday) {
					community[id1].schedule[17+k] = business[l].id;
				}
			}
		}
	}

	// if the person is unemployed
	else {
		// select 3 different time slots between 8 and 19, put them in temp array
		for (i1 = 1; i1 <= 3; ++i1) { temp[i1] = (int) uni(8,19+1); }
		while(temp[1] == temp[2] || temp[1] == temp[3] || temp[2] == temp[3]) {
			temp[2] = (int) uni(8,19+1); temp[3] = (int) uni(8,19+1);
		}
		if (temp[2] > temp[3]){prov = temp[3];temp[3]=temp[2];temp[2]=prov;}//sort the errand hours
		if (temp[1] > temp[2] && temp[1] < temp[3]){prov = temp[2];temp[2]=temp[1];temp[1]=prov;}//sort the errand hours
		if (temp[1] > temp[3]){prov = temp[1];temp[1]=temp[2];temp[2]=temp[3];temp[3]=prov;}//sort the errand hours

		// Determine the contact assignment in the errand hours
	    r = ceil(uni(0,6));
	    switch (r)
	    {
	    case 1:
		    c[1] = 2;
            c[2] = 0;
		    c[3] = 0;
		    break;
	    case 2:
		    c[1] = 1;
            c[2] = 1;
		    c[3] = 0;
		    break;
	    case 3:
		    c[1] = 0;
            c[2] = 1;
		    c[3] = 1;
		    break;
	    case 4: 
            c[1] = 0;
            c[2] = 2;
		    c[3] = 0;
		    break;
	    case 5:
		    c[1] = 0;
            c[2] = 0;
		    c[3] = 2;
		    break;
	    case 6:
		    c[1] = 1;
            c[2] = 0;
		    c[3] = 1;
		    break;
	    }

		// send the person to "afterwork" errands j = 1, 2, 3
		for (i2 = 1; i2 <= 3; ++i2) {
			float y = uni(0, 1);
			// fish out y in the cum % of weekday errands in business entities
            low = 1; 
			high = number_businesses;
			mid = (low + high)/2;
			while ( high - low >= 5) {
				if( y <= business[mid].weekday ){
					high = mid;
					mid = (low + high)/2;
				}
				if( y > business[mid].weekday ){
					low = mid;
					mid = (low + high)/2;
				}
			}
						
			for (i3 = low; i3 <= high; ++i3) {
				if (y >= business[i3-1].weekday && y < business[i3].weekday) {
					community[id1].schedule[temp[i2]] = business[i3].id;
					community[id1].errand[i2][1] = temp[i2]; 		 //record the errand schedule 
					community[id1].errand[i2][2] = business[i3].id;  //record the associated business
                    community[id1].errand[i2][3] = c[i2];            //record the associated number of contacts
				}
			}
            //fprintf(output1," id: %d  errand_hour: %d  errand_hour_rec: %d schedule1: %d schedule2: %d contact_rate: %d \n",id1,i2,community[id1].errand[i2][1],community[id1].schedule[temp[i2]],community[id1].errand[i2][2],community[id1].errand[i2][3]); 
 		}
	}	
}

// **************************************************************************	
void schedule_child_weekday(int id2)
{
	int i,z,n;
	// initialize schedule to 'home' for 24 hrs
	for (i = 1; i <= 24; ++i) community[id2].schedule[i] = 1;

	// send the child to his school from 8 to 15
	for (i = 8; i <= 15; ++i) community[id2].schedule[i] = community[id2].workplace;

	// look for the afterschool center (type t = 8) where the child is going to be
	z = (int)uni(0,workplaces[9][2]);

	for (i = 1; i <= number_businesses; ++i) {
		if (business[i].type == 8) {
			n = business[i+z].id;
			i = number_businesses;
		}
	}
	// put the afterschool center on the child's schedule
	for (i = 1; i <= 4; ++i) community[id2].schedule[15+i] = n;
}

// **************************************************************************	
void schedule_weekend(int id3)
{
	int i,j,prov;
    int temp[3+1];
	float y;
	int r;
	int c[3+1];
	int low, mid, high;

	// initialize schedule to 'home' for 24 hrs
	for (i = 1; i <= 24; ++i) {
		community[id3].schedule[i] = 1;
	}
	// select 3 different time slots between 10 and 20, put them in temp array		
		for (i = 1; i <= 3; ++i) { temp[i] = (int)uni(10,20+1); }
		while ( (temp[1] == temp[2]) || (temp[1] == temp[3] ) || (temp[2] == temp[3]) ) {
			temp[2] = (int)uni(10,20+1); temp[3] = (int)uni(10,20+1);
		}
		if (temp[2] > temp[3]){prov = temp[3];temp[3]=temp[2];temp[2]=prov;}//sort the errand hours
		if (temp[1] > temp[2] && temp[1] < temp[3]){prov = temp[2];temp[2]=temp[1];temp[1]=prov;}//sort the errand hours
		if (temp[1] > temp[3]){prov = temp[1];temp[1]=temp[2];temp[2]=temp[3];temp[3]=prov;}//sort the errand hours

		// Determine the contact assignment in the errand hours
	    r = ceil(uni(0,6));
	    switch (r)
	    {
	    case 1:
		    c[1] = 2;
            c[2] = 0;
		    c[3] = 0;
		    break;
	    case 2:
		    c[1] = 1;
            c[2] = 1;
		    c[3] = 0;
		    break;
	    case 3:
		    c[1] = 0;
            c[2] = 1;
		    c[3] = 1;
		    break;
	    case 4: 
            c[1] = 0;
            c[2] = 2;
		    c[3] = 0;
		    break;
	    case 5:
		    c[1] = 0;
            c[2] = 0;
		    c[3] = 2;
		    break;
	    case 6:
		    c[1] = 1;
            c[2] = 0;
		    c[3] = 1;
		    break;
	    }

	// send the person to weekend errands j = 1, 2, 3
		for (j = 1; j <= 3; ++j) {
			y = uni(0, 1);

			low = 1; 
			high = number_businesses;
			mid = (low + high)/2;
			while ( high - low >= 5) {
				if( y <= business[mid].weekend ){
					high = mid;
					mid = (low + high)/2;
				}
				if( y > business[mid].weekend ){
					low = mid;
					mid = (low + high)/2;
				}
			}

			for (i = low; i <= high; ++i) {
				if (y >= business[i-1].weekend && y < business[i].weekend) {
					community[id3].schedule[temp[j]] = business[i].id;
					community[id3].errandw[j][1] = temp[j]; 		 //record the errand schedule 
					community[id3].errandw[j][2] = business[i].id;  //record the associated business
                    community[id3].errandw[j][3] = c[j];            //record the associated number of contacts
				}
			}
			//fprintf(output1," id: %d  errand_hour: %d  errand_hour_rec: %d schedule1: %d schedule2: %d contact_rate: %d \n",id3,j,community[id3].errandw[j][1],community[id3].schedule[temp[j]],community[id3].errandw[j][2],community[id3].errandw[j][3]); 
		}
}

// **************************************************************************	
void disease_progress(int hour) {
	unsigned long  i;
	for (i = 1; i <= N; ++i) {
		if(community[i].infection_day_pandemic == -6){
			community[i].disease_clock_pandemic = 0;
		}

		if(community[i].infection_day_seasonal == -6){
			community[i].disease_clock_seasonal = 0;
		}

		if (community[i].disease_clock_pandemic >=0 && community[i].disease_clock_pandemic < culmination_period && community[i].infection_day_pandemic>0) {
			++community[i].disease_clock_pandemic;
			community[i].infection_day_pandemic = ceil(community[i].disease_clock_pandemic/(24+1))+1;
			//fprintf(output1," Hour: %d  Infected: %d  disease_clock: %d infection_day: %d \n",hour,i,community[i].disease_clock,community[i].infection_day);
		}

		if (community[i].disease_clock_seasonal >=0 && community[i].disease_clock_seasonal < culmination_period && community[i].infection_day_seasonal>0) {
			++community[i].disease_clock_seasonal;
			community[i].infection_day_seasonal = ceil(community[i].disease_clock_seasonal/(24+1))+1;
			//fprintf(output1," Hour: %d  Infected: %d  disease_clock: %d infection_day: %d \n",hour,i,community[i].disease_clock,community[i].infection_day);
		}

		if (community[i].disease_clock_pandemic >= 0 && community[i].disease_clock_pandemic < culmination_period && community[i].infection_day_pandemic == -2 && hour == 8) {
			++community[i].disease_clock_pandemic;
            community[i].infection_day_pandemic = ceil(community[i].disease_clock_pandemic/(24+1))+1;
			//fprintf(output1," Hour: %d  Infected: %d  disease_clock: %d infection_day: %d \n",hour,i,community[i].disease_clock,community[i].infection_day);
		}

		if (community[i].disease_clock_seasonal >= 0 && community[i].disease_clock_seasonal < culmination_period && community[i].infection_day_seasonal == -2 && hour == 8) {
			++community[i].disease_clock_seasonal;
            community[i].infection_day_seasonal = ceil(community[i].disease_clock_seasonal/(24+1))+1;
			//fprintf(output1," Hour: %d  Infected: %d  disease_clock: %d infection_day: %d \n",hour,i,community[i].disease_clock,community[i].infection_day);
		}

		if (community[i].disease_clock_pandemic >= culmination_period) {// recovers
			recover_process_pandemic(i);
		}

		if (community[i].disease_clock_seasonal >= culmination_period) {// recovers
			recover_process_seasonal(i);
		}

		if (community[i].infection_day_pandemic == -5 && community[i].disease_clock_pandemic == -3){
			++new_recovered_today_pandemic;
		}
	    if (community[i].infection_day_seasonal == -5 && community[i].disease_clock_seasonal == -3){
			++new_recovered_today_seasonal;
		}
	}	
}

// **************************************************************************
void recover_process_pandemic(unsigned long i)
{
	    community[i].disease_clock_pandemic = -3;
		community[i].infection_day_pandemic = -5;
}

// **************************************************************************
void recover_process_seasonal(unsigned long i)
{
	    community[i].disease_clock_seasonal = -3;
		community[i].infection_day_seasonal = -5;
}

// **************************************************************************
void declare_location(int hour) 
{
    unsigned long a,b,d,e,f,i,j,k,l;

// initialize households and businesses

	for (a=1; a<=(number_population_center*number_household); ++a){
		household[a].count_total =0;
		household[a].count_infected =0;
		for (e=0; e<=household_capacity-1; ++e){
			for (f = 0; f <= 1; ++f){
            household[a].array_id[e][f] = 0;
			}
		}
	}

	for (i = 2; i <= number_businesses; ++i) {
		business[i].count_total = 0; 
		business[i].count_infected = 0;
		for (j = 1; j <= business_capacity-1; ++j) {
			business[i].array_id[j][2] = 0;
		}
	}

	//take every person, find his location at current hour, put his id in the location's array
	for (i = 1; i <= N; ++i) {
		// b corresponds to the particular id of the business
		b = community[i].schedule[hour];
		d = community[i].household_number;
		// Since the number of slots in the array is business_capacity, including 0, we only can assign business_capacity-1 persons
		if (b >= 2 && (business[b].count_total <= business_capacity-1)) {
			for (k = 1; k <= business_capacity-1; ++k) {
				if (business[b].array_id[k][2] == 0) {
					business[b].array_id[k][2] = community[i].id;
					k = business_capacity+1; 
				}
			}
			++business[b].count_total;
		}

		else if (b == 1 && (household[d].count_total <= household_capacity-1)){
			for (l = 1; l<=household_capacity-1; ++l){
				if (household[d].array_id[l][1] == 0) {
					household[d].array_id[l][1] = community[i].id;
					l = household_capacity+1; 
				}
			}
			++household[d].count_total;
		}

	//	else if (b >= 2 && (business[b].count_total > business_capacity-1)) {
	//		printf("Avise %d \n", b);
	//	}
	}
}

// **************************************************************************
void  disease_spread(int hour,int day)
{
	unsigned long i,b;
	int d,f,cr,cr2,z,h;
	//float prob;

	// Contact and infection routines
    for (i = 1; i <= N; ++i) {
	// b corresponds to the particular id of the business
	b = community[i].schedule[hour];
	d = community[i].household_number;
	//Determine whether individuals are symptomatic or not and their  infectiousness profile
		if ((community[i].disease_clock_pandemic >= 1 && community[i].disease_clock_pandemic < culmination_period && community[i].infection_day_pandemic>0) || (community[i].disease_clock_seasonal >= 1 && community[i].disease_clock_seasonal < culmination_period && community[i].infection_day_seasonal>0)){
			if (community[i].symptomatic==0 && community[i].virus==1){// for the individuals initiating the outbreak
				profile_assignment_pandemic(i);	
			}
			if (community[i].symptomatic==0 && community[i].virus==2){// for the individuals initiating the outbreak
				profile_assignment_seasonal(i);	
			}

			//Determine the number of contacts and randomly mark the contacts
            
			//Weekday
			if (day%7 != 0 && day%7 != 6){
                //community[i].contact_counter = 0;
				if (community[i].workplace != 1){
					//Employees
				    if (community[i].age > 22){	
						if (hour == 8){ // Initial workday hour
							cr = business[b].contact_rate;
							if (business[b].count_total <= cr){
								cr = business[b].count_total - 1;
						    }
							if (cr > 0){//cr is equal to zero when the workplace only has the infected individual
								for (f = 1; f <= cr; ++f){
									cr2 = contact_business(i,b);
									community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_wp;//storing the proportion of duration and closeness of the workplace contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
				                }
								community[i].contact_counter = community[i].contact_counter + cr;
								community[i].contact_counter_wp = community[i].contact_counter_wp + cr;
						    }
						}
						if (hour == 18){  // Initial errand hour
							cr = business[b].contact_rate;
						    if (business[b].count_total <= cr){
								cr = business[b].count_total - 1;
						    }
							if (cr > 0){
								for (f = 1; f <= cr; ++f){
									cr2 = contact_business(i,b);
									community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
								}
								community[i].contact_counter = community[i].contact_counter + cr;
								community[i].contact_counter_er = community[i].contact_counter_er + cr;
							}
						}
						if (hour == 20){  // Initial hour at home
							cr = business[1].contact_rate;
                            if (household[d].count_total <= cr){
								cr = household[d].count_total - 1;
						    }
							if (cr > 0){
								for (f = 1; f <= cr; ++f){
									cr2 = contact_household(i,d);
									community[i].contact_array_id[community[i].contact_counter+f][1]= household[d].array_id[cr2][1];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_hh;//storing the proportion of duration and closeness of the household contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);							    
				                }
                                community[i].contact_counter = community[i].contact_counter + cr;
                                community[i].contact_counter_hh = community[i].contact_counter_hh + cr;

								for (z = 1; z <= community[i].contact_counter; ++z){
									infection (community[i].contact_array_id[z][1], community[i].contact_array_id[z][2], day, i, community[i].contact_counter_hh, community[i].contact_counter_wp, community[i].contact_counter_er);
									h = community[i].contact_array_id[z][1];
                                    //fprintf(output2,"%d %d %d %d %d %d %d %d %d %d %d %d \n",day,hour,i,community[i].contact_counter,community[i].infection_day,h,community[h].infection_day,community[h].generation,community[h].contactor_id,community[i].rn,community[i].symptomatic,community[i].profile);
							    }
                                //community[i].contact_counter = 0;
						    }
							community[i].contact_counter = 0;
							community[i].contact_counter_hh = 0;
							community[i].contact_counter_wp = 0;
							community[i].contact_counter_er = 0;
						}
					}
					if (community[i].age <= 22){//Students
						if (hour == 8){ // Initial school hour
							cr = business[b].contact_rate;
							if (business[b].count_total <= cr){
								cr = business[b].count_total - 1;
						    }
							if (cr > 0){//cr is equal to zero when the workplace only has the infected individual
								for (f = 1; f <= cr; ++f){
									cr2 = contact_business(i,b);
									community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_wp;//storing the proportion of duration and closeness of the workplace contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
				                }
								community[i].contact_counter = community[i].contact_counter + cr;
								community[i].contact_counter_wp = community[i].contact_counter_wp + cr;
						    }
						}
						if (hour == 16){  // Initial afterschool hour
							cr = business[b].contact_rate;
							if (business[b].count_total <= cr){
								cr = business[b].count_total - 1;
						    }
							if (cr > 0){
								for (f = 1; f <= cr; ++f){
									cr2 = contact_business(i,b);
									community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
								}
								community[i].contact_counter = community[i].contact_counter + cr;
								community[i].contact_counter_er = community[i].contact_counter_er + cr;
							}
						}
						if (hour == 20){  // Initial hour at home
							cr = business[1].contact_rate;
                            if (household[d].count_total <= cr){
								cr = household[d].count_total - 1;
						    }
							if (cr > 0){
								for (f = 1; f <= cr; ++f){
									cr2 = contact_household(i,d);
									community[i].contact_array_id[community[i].contact_counter+f][1]= household[d].array_id[cr2][1];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_hh;//storing the proportion of duration and closeness of the household contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);							    
				                }
                                community[i].contact_counter = community[i].contact_counter + cr;
                                community[i].contact_counter_hh = community[i].contact_counter_hh + cr;

								for (z = 1; z <= community[i].contact_counter; ++z){
									infection (community[i].contact_array_id[z][1], community[i].contact_array_id[z][2], day, i, community[i].contact_counter_hh, community[i].contact_counter_wp, community[i].contact_counter_er);
									h = community[i].contact_array_id[z][1];
                                    //fprintf(output2,"%d %d %d %d %d %d %d %d %d %d %d %d \n",day,hour,i,community[i].contact_counter,community[i].infection_day,h,community[h].infection_day,community[h].generation,community[h].contactor_id,community[i].rn,community[i].symptomatic,community[i].profile);
							    }
						    }
							community[i].contact_counter = 0;
							community[i].contact_counter_hh = 0;
                            community[i].contact_counter_wp = 0;
							community[i].contact_counter_er = 0;
						}
					}
				}
				else{//Unemployed
					if (hour == community[i].errand[1][1]){//First errand hour
						cr = business[b].contact_rate;
						if (business[b].count_total <= cr){
							cr = business[b].count_total - 1;
						}
						if (cr > 0){
							for (f = 1; f <= cr; ++f){
								cr2 = contact_business(i,b);
								community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
								community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
								//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
							}
							community[i].contact_counter = community[i].contact_counter + cr;
							community[i].contact_counter_er = community[i].contact_counter_er + cr;
						}
					}
					if (hour == community[i].errand[2][1]){//Second errand hour
						cr = business[b].contact_rate;
						if (business[b].count_total <= cr){
							cr = business[b].count_total - 1;
						}
						if (cr > 0){
							for (f = 1; f <= cr; ++f){
								cr2 = contact_business(i,b);
								community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
								community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
								//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
							}
							community[i].contact_counter = community[i].contact_counter + cr;
							community[i].contact_counter_er = community[i].contact_counter_er + cr;
						}
					}
					if (hour == community[i].errand[3][1]){//Third errand hour
						cr = business[b].contact_rate;
						if (business[b].count_total <= cr){
							cr = business[b].count_total - 1;
						}
						if (cr > 0){
							for (f = 1; f <= cr; ++f){
								cr2 = contact_business(i,b);
								community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
								community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
								//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
							}
							community[i].contact_counter = community[i].contact_counter + cr;
							community[i].contact_counter_er = community[i].contact_counter_er + cr;
						}
					}
					if (hour == 20){  // Initial hour at home
							cr = business[1].contact_rate;
                            if (household[d].count_total <= cr){
								cr = household[d].count_total - 1;
						    }
							if (cr > 0){
								for (f = 1; f <= cr; ++f){
									cr2 = contact_household(i,d);
									community[i].contact_array_id[community[i].contact_counter+f][1]= household[d].array_id[cr2][1];
									community[i].contact_array_id[community[i].contact_counter+f][2]= k_hh;//storing the proportion of duration and closeness of the hosuehold contact
									//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);							    
				                }
                                community[i].contact_counter = community[i].contact_counter + cr;
								community[i].contact_counter_hh = community[i].contact_counter_hh + cr;

								for (z = 1; z <= community[i].contact_counter; ++z){
									infection (community[i].contact_array_id[z][1], community[i].contact_array_id[z][2], day, i, community[i].contact_counter_hh, community[i].contact_counter_wp, community[i].contact_counter_er);
									h = community[i].contact_array_id[z][1];
                                    //fprintf(output2,"%d %d %d %d %d %d %d %d %d %d %d %d \n",day,hour,i,community[i].contact_counter,community[i].infection_day,h,community[h].infection_day,community[h].generation,community[h].contactor_id,community[i].rn,community[i].symptomatic,community[i].profile);
							    }
						    }
						community[i].contact_counter = 0;
						community[i].contact_counter_hh = 0;
						community[i].contact_counter_wp = 0;
						community[i].contact_counter_er = 0;
					}
				}
			}	
			else{//Weekend
				if (hour == community[i].errandw[1][1]){//First errand hour
					cr = business[b].contact_rate;
					if (business[b].count_total <= cr){
						cr = business[b].count_total - 1;
					}
					if (cr > 0){
							for (f = 1; f <= cr; ++f){
								cr2 = contact_business(i,b);
								community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
								community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
								//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
							}
						community[i].contact_counter = community[i].contact_counter + cr;
						community[i].contact_counter_er = community[i].contact_counter_er + cr;
					}
				}
				if (hour == community[i].errandw[2][1]){//Second errand hour
					cr = business[b].contact_rate;
					if (business[b].count_total <= cr){
						cr = business[b].count_total - 1;
					}
					if (cr > 0){
						for (f = 1; f <= cr; ++f){
							cr2 = contact_business(i,b);
							community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
							community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
							//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
						}
					    community[i].contact_counter = community[i].contact_counter + cr;
						community[i].contact_counter_er = community[i].contact_counter_er + cr;
					}
				}
				if (hour == community[i].errandw[3][1]){//Third errand hour
					cr = business[b].contact_rate;
					if (business[b].count_total <= cr){
						cr = business[b].count_total - 1;
					}
					if (cr > 0){
						for (f = 1; f <= cr; ++f){
							cr2 = contact_business(i,b);
							community[i].contact_array_id[community[i].contact_counter+f][1]= business[b].array_id[cr2][2];
							community[i].contact_array_id[community[i].contact_counter+f][2]= k_er;//storing the proportion of duration and closeness of the errand contact
							//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
						}
						community[i].contact_counter = community[i].contact_counter + cr;
						community[i].contact_counter_er = community[i].contact_counter_er + cr;
					}
				}
				if (hour == 21){  // Initial hour at home
					cr = business[1].contact_rate;
					if (household[d].count_total <= cr){
						cr = household[d].count_total - 1;
					}
					if (cr > 0){
						for (f = 1; f <= cr; ++f){
							cr2 = contact_household(i,d);
							community[i].contact_array_id[community[i].contact_counter+f][1]= household[d].array_id[cr2][1];
							community[i].contact_array_id[community[i].contact_counter+f][2]= k_hh;//storing the proportion of duration and closeness of the household contact
							//fprintf(output1," Day: %d  Hour: %d  Contactor: %d  No contacts: %d Business id: %d Contact in business: %d Contact id: %d Vector position: %d \n",day,hour,i,cr,b,cr2,community[i].contact_array_id[community[i].contact_counter+f][1],community[i].contact_counter+f);
						}
						community[i].contact_counter = community[i].contact_counter + cr;
						community[i].contact_counter_hh = community[i].contact_counter_hh + cr;

						for (z = 1; z <= community[i].contact_counter; ++z){
							infection (community[i].contact_array_id[z][1], community[i].contact_array_id[z][2], day, i, community[i].contact_counter_hh, community[i].contact_counter_wp, community[i].contact_counter_er);
							h = community[i].contact_array_id[z][1];
							//fprintf(output2,"%d %d %d %d %d %d %d %d %d %d %d %d \n",day,hour,i,community[i].contact_counter,community[i].infection_day,h,community[h].infection_day,community[h].generation,community[h].contactor_id,community[i].rn,community[i].symptomatic,community[i].profile);
						}
					}
				   community[i].contact_counter = 0;
				   community[i].contact_counter_hh = 0;
                   community[i].contact_counter_wp = 0;
				   community[i].contact_counter_er = 0;
				}
			}
		}
	}
}

// **************************************************************************
void profile_assignment_pandemic(unsigned long i)
{
	if (uni(0,1)<=percent_symptomatic){
				community[i].symptomatic = 1;
				community[i].profile_pandemic = ceil(uni(0,3));
				//printf("The symptomatic status is %d and the profile is %d\n", community[i].symptomatic, community[i].profile);
			    }
			    else {
				community[i].symptomatic = 2;
                community[i].profile_pandemic = ceil(uni(3,6));
				//printf("The symptomatic status is %d and the profile is %d\n", community[i].symptomatic, community[i].profile);
			    }
}
// **************************************************************************
void profile_assignment_seasonal(unsigned long i)
{
	if (uni(0,1)<=percent_symptomatic){
				community[i].symptomatic = 1;
				community[i].profile_seasonal = ceil(uni(0,3));
				//printf("The symptomatic status is %d and the profile is %d\n", community[i].symptomatic, community[i].profile);
			    }
			    else {
				community[i].symptomatic = 2;
                community[i].profile_seasonal = ceil(uni(3,6));
				//printf("The symptomatic status is %d and the profile is %d\n", community[i].symptomatic, community[i].profile);
			    }
}
// **************************************************************************
int contact_business (unsigned long entity, int entity_location_hour)
{
	int cr2;
	cr2 = ceil(uni(0,business[entity_location_hour].count_total));
	while (business[entity_location_hour].array_id[cr2][2] == entity){// To avoid the individual contacts him/herself or when only him/herself works in the business
	cr2 = ceil(uni(0,business[entity_location_hour].count_total));
	}
	//community[business[entity_location_hour].array_id[cr2][2]].contact = 5;
    //community[business[entity_location_hour].array_id[cr2][2]].contactor_id = community[entity].id;
	return cr2;
}
// **************************************************************************
int contact_household (unsigned long entity, int entity_location_hour)
{
	int cr2;
	cr2 = ceil(uni(0,household[entity_location_hour].count_total));
    while (household[entity_location_hour].array_id[cr2][1] == entity){// To avoid the individual contacts or him/herself when only him/herself lives in the household
	cr2 = ceil(uni(0,household[entity_location_hour].count_total));
	}
	//community[household[entity_location_hour].array_id[cr2][1]].contact = 5;
    //community[household[entity_location_hour].array_id[cr2][1]].contactor_id = community[entity].id;
	return cr2;
}
// **************************************************************************
void  infection (unsigned long id, float k, int day, unsigned long infector, int contact_rate_hh, int contact_rate_wp, int contact_rate_er)// determine whether a person is infected or not
{
	float p,p_p,p_s,z,z_p,z_s;
	int virus;
	virus=community[infector].virus;

	p=-1;
	z=-1;

	if(virus==3){//the individual seeding infection is infected with both viruses
		if(community[infector].infection_day_pandemic==-2){
			p_p=0;
		}
		else{
			p_p = infection_probability (community[infector].rn_init_pandemic, community[infector].profile_pandemic, community[infector].infection_day_pandemic, contact_rate_hh, contact_rate_wp, contact_rate_er);
		}
		z_p = uni(0,1);

		if(community[infector].infection_day_seasonal==-2){
			p_s=0;
		}
		else{
		p_s = infection_probability (community[infector].rn_init_seasonal, community[infector].profile_seasonal, community[infector].infection_day_seasonal, contact_rate_hh, contact_rate_wp, contact_rate_er);
		}
		z_s = uni(0,1);

		if (z_p <= p_p && z_s > p_s){//infected from pandemic
			virus = 1;
			p = p_p;
			z = z_p;
		}
		if (z_s <= p_s && z_p > p_p){//infected from seasonal
			virus = 2;
			p = p_s;
			z = z_s;
		}
		if (z_p <= p_p && z_s <= p_s){
			if (community[id].prior_virus==0){//no prior infection, infected with pandemic and seasonal
				profile_assignment_pandemic(id);
				profile_assignment_seasonal(id);
				community[id].disease_clock_pandemic = 0;
				community[id].disease_clock_seasonal = 0;
				community[id].generation_pandemic = community[infector].generation_pandemic + 1;
				community[id].generation_seasonal = community[infector].generation_seasonal + 1;
				community[infector].rn_pandemic = community[infector].rn_pandemic + 1;
				community[infector].rn_seasonal = community[infector].rn_seasonal + 1;
				community[id].infection_day_pandemic = -2;
				community[id].infection_day_seasonal = -2;
				community[id].day_begin_infection_pandemic = day;
				community[id].day_begin_infection_seasonal = day;
				community[id].virus = 3;
				community[id].prior_virus = 3;
				community[id].rn_init_pandemic = reproduction_number_pandemic*epsilon_ps;
				community[id].rn_init_seasonal = reproduction_number_seasonal*epsilon_sp;
				++total_infected_pandemic;
		        ++new_infected_today_pandemic;
			    ++total_infected_seasonal;
		        ++new_infected_today_seasonal;
				++total_sim_coinfected;
		        ++new_sim_coinfected;
			}
			if (community[id].prior_virus==2 && community[id].disease_clock_seasonal>=0){//Infected with seasonal, still in the seasonal process and infected with both pandemic and seasonal
				virus = 1;//The person will get infected from pandemic only
			    p = p_p;
			    z = z_p;
			}
			if (community[id].prior_virus==2 && community[id].disease_clock_seasonal==-3 && community[id].infection_day_seasonal==-5){//Infected with seasonal, recovered from seasonal process and re-infected with both pandemic and seasonal
				virus = 1;//The person will get infected from pandemic only
			    p = p_p;
			    z = z_p;
			}
			if (community[id].prior_virus==1 && community[id].disease_clock_pandemic>=0){//Infected with pandemic, still in the pandemic process and infected with both pandemic and seasonal
				virus = 2;//The person will get infected from seasonal only
			    p = p_s;
			    z = z_s;
	     	}
			if (community[id].prior_virus==1 && community[id].disease_clock_pandemic==-3 && community[id].infection_day_pandemic==-5){//Infected with pandemic, recovered from pandemic process and re-infected with both pandemic and seasonal
				virus = 2;//The persona will get infected from seasonal only
			    p = p_s;
			    z = z_s;
			}
		}
	}

	if (virus==1){
		if (p==-1){		
		p = infection_probability (community[infector].rn_init_pandemic, community[infector].profile_pandemic, community[infector].infection_day_pandemic, contact_rate_hh, contact_rate_wp, contact_rate_er);
		}
		if (z==-1){
		z= uni(0,1);
		}
		if (z <= (k*p)){
			if (community[id].prior_virus==0){//no prior infection, infected with pandemic
				profile_assignment_pandemic(id);
				community[id].disease_clock_pandemic = 0;
				community[id].generation_pandemic = community[infector].generation_pandemic + 1;
				community[infector].rn_pandemic = community[infector].rn_pandemic + 1;
				community[id].infection_day_pandemic = -2;
				community[id].day_begin_infection_pandemic = day;
				community[id].virus = 1;
                community[id].prior_virus = 1;
				community[id].rn_init_pandemic = reproduction_number_pandemic;
				++total_infected_pandemic;
		        ++new_infected_today_pandemic;
				++total_infected_pandemic_only;
		        ++new_infected_today_pandemic_only;
			}
			//if (community[id].prior_virus==2 && community[id].disease_clock_seasonal>=0 && community[id].infection_day_seasonal>0){//Infected with seasonal, still in the seasonal process and infected with pandemic				
			if (community[id].prior_virus==2 && community[id].disease_clock_seasonal>=0){//Infected with seasonal, still in the seasonal process and infected with pandemic	
			    profile_assignment_pandemic(id);
				community[id].disease_clock_pandemic = 0;
				community[id].generation_pandemic = community[infector].generation_pandemic + 1;
				community[infector].rn_pandemic = community[infector].rn_pandemic + 1;
				community[id].infection_day_pandemic = -2;
				community[id].day_begin_infection_pandemic = day;
				community[id].virus = 3;
                community[id].prior_virus = 3;
                community[id].rn_init_pandemic = reproduction_number_pandemic*epsilon_p;
				++total_infected_pandemic;
		        ++new_infected_today_pandemic;
				++total_coinfected_s_p;
		        ++new_coinfected_s_p;
		    }
			if (community[id].prior_virus==2 && community[id].disease_clock_seasonal==-3 && community[id].infection_day_seasonal==-5){//Infected with seasonal, recovered from seasonal process and infected with pandemic
			    profile_assignment_pandemic(id);
				community[id].disease_clock_pandemic = 0;
				community[id].generation_pandemic = community[infector].generation_pandemic + 1;
				community[infector].rn_pandemic = community[infector].rn_pandemic + 1;
				community[id].infection_day_pandemic = -2;
				community[id].day_begin_infection_pandemic = day;
				community[id].virus = 1;
                community[id].prior_virus = 3;
                community[id].rn_init_pandemic = reproduction_number_pandemic*epsilon_pr;
				++total_infected_pandemic;
		        ++new_infected_today_pandemic;
				++total_reinfected_s_p;
		        ++new_reinfected_s_p;
		    }
			if(community[id].infection_day_pandemic==-6){
				printf("La persona es %d \n", id);
			}

		}
	}
	if (virus==2){
		if (p==-1){
		p = infection_probability (community[infector].rn_init_seasonal, community[infector].profile_seasonal, community[infector].infection_day_seasonal, contact_rate_hh, contact_rate_wp, contact_rate_er);
		}
        if (z==-1){
		z= uni(0,1);
		}
		if (z <= (k*p)){
			if (community[id].prior_virus==0){//no prior infection, infected with seasonal
				profile_assignment_seasonal(id);
				community[id].disease_clock_seasonal = 0;
				community[id].generation_seasonal = community[infector].generation_seasonal + 1;
				community[infector].rn_seasonal = community[infector].rn_seasonal + 1;
				community[id].infection_day_seasonal = -2;
				community[id].day_begin_infection_seasonal = day;
				community[id].virus = 2;
				community[id].prior_virus = 2;
				community[id].rn_init_seasonal = reproduction_number_seasonal;
			    ++total_infected_seasonal;
		        ++new_infected_today_seasonal;
				++total_infected_seasonal_only;
		        ++new_infected_today_seasonal_only;
			}
			if (community[id].prior_virus==1 && community[id].disease_clock_pandemic>=0){//Infected with pandemic, still in the pandemic process and infected with seasonal
				profile_assignment_seasonal(id);
				community[id].disease_clock_seasonal = 0;
				community[id].generation_seasonal = community[infector].generation_seasonal + 1;
				community[infector].rn_seasonal = community[infector].rn_seasonal + 1;
				community[id].infection_day_seasonal = -2;
				community[id].day_begin_infection_seasonal = day;
				community[id].virus = 3;
				community[id].prior_virus = 3;
				community[id].rn_init_seasonal = reproduction_number_seasonal*epsilon_s;
				++total_infected_seasonal;
		        ++new_infected_today_seasonal;
				++total_coinfected_p_s;
		        ++new_coinfected_p_s;
			}
			if (community[id].prior_virus==1 && community[id].disease_clock_pandemic==-3 && community[id].infection_day_pandemic==-5){//Infected with seasonal, recovered from seasonal process and infected with pandemic
			    profile_assignment_seasonal(id);
				community[id].disease_clock_seasonal = 0;
				community[id].generation_seasonal = community[infector].generation_seasonal + 1;
				community[infector].rn_seasonal = community[infector].rn_seasonal + 1;
				community[id].infection_day_seasonal = -2;
				community[id].day_begin_infection_seasonal = day;
				community[id].virus = 2;
                community[id].prior_virus = 3;
                community[id].rn_init_seasonal = reproduction_number_seasonal*epsilon_sr;
				++total_infected_seasonal;
		        ++new_infected_today_seasonal;
				++total_reinfected_p_s;
		        ++new_reinfected_p_s;
		    }
			if(community[id].infection_day_seasonal==-6){
				printf("La persona es %d \n", id);
			}
		}
	}
}
// **************************************************************************
float  infection_probability (float repnum, int profile, int infection_day, int contact_rate_hh, int contact_rate_wp, int contact_rate_er)
{
	float a,prob;
	switch (profile)
	{
		case 1:
		a = gamma1[infection_day][1];
        //prob = ((repnum*a*(1+((1-asymp)/((percent_symptomatic)/(1-percent_symptomatic)))))/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er))); This used to go for case 1, case 2 and case 3
        prob = ((repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	case 2:
		a = lognorm1[infection_day][1];
        prob = ((repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	case 3:
		a = weib1[infection_day][1];
        prob = ((repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	case 4: 
        a = gamma2[infection_day][1];
		//prob = (repnum*a*asymp)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		prob = (asymp*(repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	case 5:
		a = lognorm2[infection_day][1];
		prob = (asymp*(repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	case 6:
		a = weib2[infection_day][1];
		prob = (asymp*(repnum/(((1-asymp)*percent_symptomatic)+asymp))*a)/((contact_rate_hh*k_hh)+(contact_rate_wp*k_wp)+(contact_rate_er*k_er));
		break;
	}
	return prob;
}
// **************************************************************************
void check()
{   int aux, i;
	int aux_size_p = 0, aux_size_s = 0;
	int aux_sumrn_p = 0, aux_sumrn_s = 0;
	float aux_average_rn_p = 0, aux_average_rn_s  = 0;

	output4 = fopen("output_R.txt","w");
	fprintf(output4, "Generation   Generation_size_p   Avg_R_p   Generation_size_s   Avg_R_s   \n");

	for(aux=1; aux <= 10; ++ aux){ //max_days
		for(i = 1; i <=N; ++i){
			if(community[i].generation_pandemic==aux){
				++aux_size_p;
				aux_sumrn_p = aux_sumrn_p + community[i].rn_pandemic;	
			}
			if(community[i].generation_seasonal==aux){
				++aux_size_s;
				aux_sumrn_s = aux_sumrn_s + community[i].rn_seasonal;	
			}
		}
		aux_average_rn_p=  (float)aux_sumrn_p/aux_size_p;
		aux_average_rn_s=  (float)aux_sumrn_s/aux_size_s;
		fprintf(output4,"%d	   %d	 %f    %d    %f \n", aux, aux_size_p, aux_average_rn_p, aux_size_s, aux_average_rn_s);
		aux_size_p = 0;
		aux_sumrn_p = 0;
		aux_average_rn_p = 0;
		aux_size_s = 0;
		aux_sumrn_s = 0;
		aux_average_rn_s = 0;
	}

	fclose(output4);
}
// **************************************************************************
float random1(void)
  //  creates iid random variates uniformly distributed between 0 and 1
  //  SIMAN'S olg generation number.
  //  Possible seeds
  //  Original ALEX  19730613
  //  others 2003443118, 1058408196, 1897948857, 151962393, 18284570280, 741436682, 1232943115, 34622975, 5208903
  //  others 123294311, 3976587, 40392123, 445307, 11429420, 4455931, 92227904, 121657947, 22890110, 90160696
{
		//static long seed=34622975;
		long i=seed/127773;
		long j=seed%127773;
		long k=j*16807-i*2836;
		if (k>0) seed=k;
		else seed=k+2147483647;

		return seed/2147483647.0;
}

// **************************************************************************	
float uni(unsigned long a, unsigned long b)
{   
	if (uniform_counter <= 9998) {
		++uniform_counter;
		return a + (b-a)*uniform[uniform_counter];
	}
	else {
		uniform_counter = 1;
		return a + (b-a)*uniform[uniform_counter];
	}
}
