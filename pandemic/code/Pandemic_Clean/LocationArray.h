#pragma once
#include <vector>
#include "Location.h"
#include "Person.h"
using namespace std;

class LocationArray
{
public:
	LocationArray(void);
	LocationArray(int);
	~LocationArray(void);

	Location * getLocation(int);
	void insertPerson(Person *, int loc);

	void Empty();
	void resize(int);

	static void copy_loc_data(LocationArray source, LocationArray dest);

	vector<Location> locs;

};

