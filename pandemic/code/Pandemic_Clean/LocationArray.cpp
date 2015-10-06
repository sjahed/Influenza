#include "LocationArray.h"
#include <iostream>

LocationArray::LocationArray(void)
{
}

LocationArray::LocationArray(int num_locs)
{
	std::cout<<"LocationArray::LocationArray\t";
	locs.resize(num_locs);
}


LocationArray::~LocationArray(void)
{
}

Location * LocationArray::getLocation(int i)
{
	// std::cout<<"LocationArray::getLocation\n";
	return &locs.at(i);
}

//removes all people from the location array, keeping location data
void LocationArray::Empty()
{
	for(int i = 0; i < locs.size(); i++)
		locs.at(i).Empty();
	//std::cout<<"LocationArray::Empty\n";
}
void LocationArray::resize(int new_size)
{
	//std::cout<<"LocationArray::resize\n";
	locs.resize(new_size);
}


void LocationArray::insertPerson(Person * p, int loc)
{
	std::cout<<"loc="<<loc<<"p=";
	locs.at(loc).insert_person(p);

}