#pragma once
#include <omp.h>
#include "Person.h"
#include <vector>
using namespace std;
#define LOCATION_MAX 10

struct LocationData
{
	short int location_type;
	short int business_type;
	int max_contacts;
};

class Location
{
public:
	Location(void);
	~Location(void);
	void insert_person(Person * p);
	Person * get_person(int idx);
	int count_hourly_total;

//	short int location_type;
//	short int business_type;
//	int contacts_max;
	void Empty();

	//  short int contacts_count;
//	Person * people[LOCATION_MAX];
	vector<Person *> peopleOverflow;
private:
	omp_lock_t mutex;
};

