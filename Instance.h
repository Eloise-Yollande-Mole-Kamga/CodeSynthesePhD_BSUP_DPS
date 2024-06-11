#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <cmath>


using namespace std;

// this file contains the Instance class:
// the Instance class contains all the input data regarding the instance
// as well as the methods to read an input data file, preprocess the data and display the data


struct Instance
{

	//time horizon data
	int _Tmax;          // time horizon               [-> TMAX]
	int _p;             // period duration            [-> p]
	int _nbPeriod;      // nb of periods              [-> N]

	// vehicle data
	int _vehH0;         // initial veh hydrogen load  [->E0]
	int _capVeh;        // vehicle tank capacity      [-> CVeh]


	// production data
	int _setupCost;          // activation cost                                [-> CostF]
	int _MPH0;               // initial micro plant hydrogen load              [->H0]
	int _capMP;              // micro plant tank capacity                      [->CMP]
	vector<int> _prodRate;   //production rates (index from 0 to nbPeriod-1)   [-> Ri]
	vector<int> _prodCost;   //production cost (index from 0 to nbPeriod-1)    [-> CostVi]


	// station data (index 0 = initial depot;  index _nbStation+1 = final depot)
	int _nbStation = 0;                            // number of stations         [-> M]
	vector< pair<int, int> > _coord;           // stations coordinates (index from 0 = depot to nbStation)
	vector< int > _time;                       // _time[j] = time to go from j to j+1 ( j = 0...nbStation) [-> t]
	vector< int > _timeToMP;                   // _timeToMP[j] = time to go from j to micro plant ( j = 0...nbStation) [-> d]
	vector< int > _timeFromMP;                 // _timeFromMP[j] = time to go from micro plant to j ( j = 0...nbStation+1) [-> d*]
	vector< int > _energy;                     // _energy[j] = energy to go from j to j+1 ( j = 0...nbStation) [-> e]
	vector< int > _energyToMP;                 // _energyToMP[j] = energy to go from j to micro plant ( j = 0...nbStation+1) (we add a dummy value for j = nbStation+1, usefull in the algo) [-> eps]
	vector< int > _energyFromMP;               // _energyFromMP[j] = energy to go from micro plant to j ( j = 0...nbStation+1) [-> eps*]


	//========================================================================================================
	//   Methods

	// read an instance contained in 2 data files (the second one must contain the production data associated with the first one)
	void readDataFile(const string & name1, const string & name2);

	// preprocess the data read to complete the instance data
	void preprocess();

	// display the instance in text format
	void display();

	// draw the instance using Python
	// input:  instance identifier
	void draw(const string & name);

	// return the total amount of H2 the micro plant can produce (if it produces no stop from period 0 to period N-1)
	int prodCapacity() const;

	//returns the time to go from MP to station next to next +1 ... to last 
	// i.e. time (MP -> next -> next + 1 -> ... -> last
	int timeSubTour(int next, int last) const;


	//returns the time to go from depot to MP after station last 
	// i.e. time (depot -> 1 -> 2 -> ... -> last -> MP)
	int timeSubTourInit(int last) const;

	//returns the time to go from MP to last to final depot 
	// i.e. time (MP -> last -> .. -> final depot)
	int timeSubTourFinal(int last) const;
};




int euclidianDistance(int x1, int y1, int x2, int y2);
int manhattanDistance(int x1, int y1, int x2, int y2);

// display each element of a integer vector in a line
void displayVector(ostream& os, vector<int>& v);

// display each element of a integer vector in a line
void displayVector(ostream& os, vector<pair<int,int> >& v);