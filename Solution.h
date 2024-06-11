#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>

#include "Instance.h"


using namespace std;



struct Solution {

	int _nbStation = -1;
	int _nbPeriod = -1;

	
	// STATIONS INDEXED VECTORS
	vector<bool> _refuelSt;           // _refuelSt[j] = true if the veh refuels after st j (j = 0, ..., _nbStation)
	vector<int> _quantity;            // _quantity[j] = quantity of H2 units refuelled by the veh after st j (j = 0, ..., _nbStation)

	// PERIODS INDEXED VECTORS
	vector<bool> _produce;            // _produce[i] = true if the micro plant produces at period i (i = 0, ..., _nbPeriod-1)
	vector<int> _refuelP;			  // _refuelP[i] = quantity of H2 the vehicle refuels during period i (i = 0, ..., _nbPeriod-1), 0 means no refuel

	// SYNCHRONISATION MATRIX
	vector< vector<bool> > _synch;    // _synch[i][j] = true if the veh refuels during period i while travelling from st j to j+1, (i = 0, ..., _nbPeriod-1), (j = 0, ..., _nbStation)

	int _prodCost;					  // production cost (including setup costs)
	int _totalTime;					  // time when the vehicle comes back to the depot


	//=====================================================
	// auxiliary data to check and draw the solution

	vector<int> _timeAtStation; // _timeAtStation[j] = date at which the vehicle arrives at station j
	vector<pair<int,int>> _timeAtMP; // _timeAtMP[i] = (arrival date, leaving date) at the micro-plant for the ith refuelling
	
	vector<int> _MPLoad; // _MPLoad[i] = quantity of hydrogen in the microplant at the beginning of period i
	vector<pair<int, int>> _vehLoad; // _vehLoad[i] = (date, quantity of hydrogen) in the vehicle tank,
									 // the dates are: arrival at a station or at the MP | beginning of a refuelling | end of a refuelling (in incresing order)


	//initialize the arrays
	void init(int M, int N);


	// ============   functions to check and visualize a solution =======================

	//fill the solution by hand (only usefull for debugging purposes)
	void fillByHand();

	//display the solution (in text format)
	void display();

	//draw the solution
	//name = name for the out file (without extension)
	void draw(const Instance& ins, const string& name);

	//check of the solution is valid: return true if it is
	bool checkValidity(const Instance & ins);
	
	
	// ============  AUXILIARY fct =======================

	//compute _timeAtStation & _timeAtMP & _vehLoad
	void computeTimeAndVehLoad(const Instance& ins);

	//compute _MPLoad  
	void computeMPLoad(const Instance& ins);

	//return the period index during which the vehicle refuels when it refuels after station j
	int refuelStToPeriod(int j);

	//return the total H2 needed by the vehicle (_quantity must be up to date)
	int sumRefuel();

	//return the next station (after j) after which the vehicle refuels
	//return M+1 if j is the latest station after which the vehicle refuels
	int nextRefuelStation(int j);

	//returns the date (in time units, not in period) at which the vehicle begins to refuel after visiting station j
	// _synch must be up to date: i.e. contains one i such that _synch[i][j] = true
	int beginRefuel(const Instance& ins, int j);


	//======================== save the solution costs and main caracteristics in a file ====================
	
	//ok = true if the solution is completely builded and valid (false if the algo did not succeed to build it)
	//insName is the instance name, used to create the file name to save the solution
	void enregistreSol(const Instance& ins, string insName, bool ok, double cpuTime);


	//======================= compute global information about the solution ==========================
	//return the number of refuelling
	int computeNbRefuel();

	//returns the number of production setups
	int computeNbSetup();

	//return the total waiting time (in time units) of the vehicle at the MP
	int computeWaitTime(const Instance& ins);

};