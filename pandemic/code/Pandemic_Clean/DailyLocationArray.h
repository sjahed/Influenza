#pragma once
#include "Person.h"
#include "Location.h"
#include "simParameters.h"

#include "LocationArray.h"

class DailyLocationArray
{
public:
	DailyLocationArray(void);
	~DailyLocationArray(void);

	void Empty();
	void resize_all(int size);
	void resize_selectHours(vector<int> hours, int number_workplaces, int number_locations);

	void insertPerson(Person * p, int locId, int weekend, int hour);

	int getLocCount(int locId, int weekend, int hour);
	Person * getPersonAtLoc(int idx, int locId, int weekend, int hour);

	LocationArray locArrays[24];

	vector<LocationData> locData;

	void setup_sizeStaticArrays(int num_hh, int num_wp);
	void setup_insertPersonStatic(Person * p);
	vector<Location> locations_hh;
	vector<Location> locations_wp;

	Location * getLoc(int locId, int weekend, int hour);
};

