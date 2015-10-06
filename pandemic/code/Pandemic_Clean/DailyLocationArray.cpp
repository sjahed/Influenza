#include "DailyLocationArray.h"
#include <algorithm>
#include <iostream>
DailyLocationArray::DailyLocationArray()
{

}

DailyLocationArray::~DailyLocationArray()
{

}

void DailyLocationArray::resize_all(int size)
{
	for(int i = 0; i < 24; i++)
		locArrays[i].resize(size);

	locData.resize(size);
}

void DailyLocationArray::resize_selectHours(vector<int> hours, int number_workplaces, int number_locations)
{
	for(int i = 0; i < hours.size(); i++)
		locArrays[hours[i]].resize(number_workplaces);

	locData.resize(number_locations);
}

void DailyLocationArray::Empty()
{
	for(int i =0; i < 24; i++)
		locArrays[i].Empty();
}

void DailyLocationArray::insertPerson(Person * p, int locId, int weekend, int hour)
{
	Location * l = getLoc(locId,weekend,hour);
	//std::cout<<locId<<".."<<weekend<<".."<<hour<<"then\n";
	l->insert_person(p);
}

int DailyLocationArray::getLocCount(int locId, int weekend, int hour)
{
	Location * l = getLoc(locId,weekend,hour);
	
	return l->count_hourly_total;
}

Person * DailyLocationArray::getPersonAtLoc(int idx, int locId, int weekend, int hour)
{
	Location * l = getLoc(locId,weekend,hour);
	return l->get_person(idx);
}

void DailyLocationArray::setup_sizeStaticArrays(int num_hh, int num_wp)
{
	locations_wp.reserve(num_wp);
	for(int i = 0; i < num_wp; i++)
	{
		locations_wp.push_back(Location());
	}

	int num_locs = num_hh + num_wp;
	locations_hh.reserve(num_locs);
	for(int i = 0; i < num_locs; i++)
	{
		locations_hh.push_back(Location());
	}

}

void DailyLocationArray::setup_insertPersonStatic(Person * p)
{
	int hh = p->household;
	int wp = p->workplace;

	locations_hh[hh].insert_person(p);
	locations_wp[wp].insert_person(p);
}

Location * DailyLocationArray::getLoc(int locId, int weekend, int hour)
{
	//if it's the home hour, return the static array
	if(hour == HOUR_HOME)
		return &locations_hh[locId];

	//if it's a weekday and the work hour, return static array
	if(!weekend && hour == HOUR_WORK_START)
		return &locations_wp[locId];

	//else return the normal array
	return locArrays[hour].getLocation(locId);
}
