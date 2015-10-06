#pragma once
#include <climits>
using namespace std;


#define MAX_MAX_DAYS 10

const int number_business_types = 14;
//#define number_business_types 14 // # of business types (including home)
const int number_adult_age_types= 3; // # of age types for adults
const int number_child_age_types= 5; // # of age types for kids
const int number_household_types= 9; // # of household types
const int number_population_center= 1; //# of population centers in the community
const int number_zipcodes= 57; //# of population centers in the community
const int number_location_types= 5;



//status codes for people
const int STATUS_UNEXPOSED= 0;
const int STATUS_SYMPTOMATIC= 1;
const int STATUS_ASYMPTOMATIC= 2;
const int STATUS_RECOVERED= 3;
const int STATUS_DEAD= 4;

//type codes for locations
const int BUSINESS= 0;
const int SCHOOL= 1;
const int HOUSEHOLD= 2;
const int ERRAND= 3;
const int AFTERSCHOOL= 4;

const int DATA_BUSINESS_TYPE_AFTERSCHOOL_IDX= 9;

//define days of the week in int form
const int DAY_MONDAY= 0;
const int DAY_TUESDAY= 1;
const int DAY_WEDNESDAY= 2;
const int DAY_THURSDAY= 3;
const int DAY_FRIDAY= 4;
const int DAY_SATURDAY= 5;
const int DAY_SUNDAY= 6;

const int CULMINATION_PERIOD= 10;

//note: all are zero indexed, hence the -1
const int HOUR_WORK_START= 8 - 1;
const int HOUR_WORK_END= 17 - 1;
const int HOUR_SCHOOL_START= 8 - 1;
const int HOUR_SCHOOL_END= 17 - 1;
const int HOUR_AFTERSCHOOL_START= 18 - 1;;
const int HOUR_AFTERSCHOOL_END= 19 - 1;
const int HOUR_HOME = 21 - 1;

const int HOUR_UNEMPLOYED_ERRAND_START= 8 - 1;
const int HOUR_UNEMPLOYED_ERRAND_END= 19 - 1;

const int HOUR_WEEKEND_ERRAND_START= 10 - 1;
const int HOUR_WEEKEND_ERRAND_END= 20 - 1;

const int HOUR_INFECTION_UPDATE= 18 - 1;

const int ACTION_RECOVER_PANDEMIC= 0;
const int ACTION_RECOVER_SEASONAL= 1;
const int ACTION_INFECT_PANDEMIC= 2;
const int ACTION_INFECT_SEASONAL= 3;
const int ACTION_INFECT_BOTH= 4;

const int DATA_WORKPLACE_TYPE_COUNT_IDX= 1;
const int DATA_WORKPLACE_WORK_PERCENT_IDX= 2;
const int DATA_WORKPLACE_WEEKDAY_ERRAND_PERCENT_IDX= 3;
const int DATA_WORKPLACE_WEEKEND_ERRAND_PERCENT_IDX= 4;
const int DATA_WORKPLACE_MAX_CONTACTS_IDX= 6;

const int DATA_CHILD_AGE_AGEVAL= 0;
const int DATA_CHILD_AGE_CDF_IDX= 1;
const int DATA_CHILD_AGE_SCHOOL_TYPE_IDX= 2;

const int DATA_PROFILE_INFECTION_DAY_IDX= 0;
const int DATA_PROFILE_CUMULATIVE_DENSITY_IDX= 1;

const int DATA_HOUSEHOLD_NUMBER_ADULTS_IDX =0;
const int DATA_HOUSEHOLD_NUMBER_CHILDREN_IDX =1;
const int DATA_HOUSEHOLD_CDF_IDX =2;

const int HOUSEHOLD_MAX_CONTACTS = 3;
