#include "Solution.h"
#include <numeric>

//initialize the arrays
void Solution::init(int M, int N)
{
	//set the number of stations and periods
	_nbStation = M;
	_nbPeriod = N;

	//allocate the arrays
	_refuelSt.resize(_nbStation + 1);
	_quantity.resize(_nbStation + 1);
	_produce.resize(_nbPeriod);
	_refuelP.resize(_nbPeriod);
	_synch.resize(_nbPeriod, vector<bool>(_nbStation + 1));
}


// ============   TO DO =======================


//fill the solution the hand (only usefull for debugging purposes)
void Solution::fillByHand()
{
	// initialize _nbStation and _nbPeriod (regarding the instance for which the solution is designed)
	// TO DO

	// allocate the arrays
	init(_nbStation, _nbPeriod);

	// fill the arrays
	// TO DO

	cout << "Solution::fillByHand : TO DO" << endl;

}



//display the solution (in text format)
void Solution::display()
{
	assert(_nbStation != -1);

	cout << " ======================   SOLUTION   =========================== " << endl;

	//refuelling solution
	for (int i = 0; i <= _nbStation; ++i)
	{
		if (_refuelSt[i])
		{
			cout << "Refuels  " << _quantity[i] << "  H2 units after station  " << i << endl;
		}
	}

	//production schedule
	for (int i = 0; i < _nbPeriod; ++i)
	{
		if (_produce[i])
		{
			cout << "Produce at period " << i << endl;
		}
	}

	//synchronisation
	for (int i = 0; i < _nbPeriod; ++i)
	{
		for (int j = 0; j <= _nbStation; ++j)
		{
			if (_synch[i][j])
			{
				cout << "refuel during period  " << i << "  after visiting station " << j << endl;
			}
		}
	
	}

	cout << "prod cost = " << _prodCost << endl;
	cout << "total travel time = " << _totalTime << endl;
}

//draw the solution
void Solution::draw(const Instance & ins, const string& name)
{
	assert(_nbStation != -1);


	//1. first export the data for python 

	ofstream fic(name + "_py.txt");

	fic << ins._Tmax << endl;
	fic << ins._nbPeriod << endl;
	fic << ins._p << endl;
	fic << ins._nbStation << endl;
	
	displayVector(fic, _timeAtStation);
	displayVector(fic, _timeAtMP);

	displayVector(fic, _MPLoad);

	//first display the dates
	for (const pair<int, int> & p : _vehLoad)
		fic << p.first << " ";
	fic << endl;

	//second display the hydrogen load for each date
	for (const pair<int, int> & p : _vehLoad)
		fic << p.second << " ";
	fic << endl;


	fic.close();

	// 2. second call python 
	string cmd = "python drawSol.py " + name + "_py";
	//system(cmd.c_str());
}


//check of the solution is valid: return true if it is
bool Solution::checkValidity(const Instance & ins)
{
	//  /!\  all the solution data including the auxiliary arrays of dates and load must be up to date
	assert(_nbStation != -1);
	assert(_timeAtStation.size() > 0);
	assert(_vehLoad.size() > 0);
	assert(_MPLoad.size() > 0);
	assert(_timeAtMP.size() > 0);

	bool ok = true;

	// 1. the MP does not produce when the vehicle refuels
	for (int i = 0; i < _nbPeriod; ++i)
	{
		if (_produce[i] && _refuelP[i] > 0)
			return false;
	}

	// 2. the hydrogen level in the MP tank is always >= 0 && <= Capacity
	for (int i = 0; i < _nbPeriod; ++i)
	{
		if (_MPLoad[i] < 0 || _MPLoad[i] > ins._capMP)
			return false;
	}

	// 3. the hydrogen level in the veh tank is always >= 0 && <= Capacity
	for (const pair<int,int> & p : _vehLoad)
	{
		if (p.second < 0 || p.second > ins._capVeh)
			return false;
	}

	// 4. the vehicle returns to the depot with at least ins._vehH0 in its tank
	if (_vehLoad[_vehLoad.size() - 1].second < ins._vehH0)
		return false;


	// 5. the MP load is at least ins._MPH0 at the end of the time horizon
	if (_MPLoad[_nbPeriod] < ins._MPH0)
		return false;

	// 6. compute again the dates and energy needed by the vehicle
	int timeCur = 0;              // arrival time at station j+1 at the end of the loop in j
	int energyCur = ins._vehH0;   // energy in veh tank at station j+1 at the end of the loop in j

	for (int j = 0; j <= _nbStation; ++j)
	{
		//go straight from j to j+1
		if (!_refuelSt[j])
		{
			timeCur += ins._time[j];       //arrival time at station j+1
			energyCur -= ins._energy[j];   //energy in veh tank at station j+1
			if (timeCur != _timeAtStation[j + 1])
				return false;
		}
		else //j -> MP -> j+1
		{
			energyCur -= ins._energyToMP[j]; //go to MP
			energyCur += _quantity[j]; //refuel
			energyCur -= ins._energyFromMP[j + 1]; //go from MP to j+1
			
			timeCur = beginRefuel(ins, j); //time at which the vehicle begins to refuel
			timeCur += ins._p; //refuel
			timeCur += ins._timeFromMP[j + 1]; //go from MP to j+1
		}
	}

	//check the total travel time
	if (timeCur != _totalTime)
		return false;

	//check the energy at the end of the tour
	if (_vehLoad[_vehLoad.size() - 1].second != energyCur)
		return false;

	// 7. check the production cost
	int costCur = 0;
	bool setup = true;
	for (int i = 0; i < _nbPeriod; ++i)
	{
		if (_produce[i])
		{
			costCur += ins._prodCost[i];
			if (setup)
			{
				costCur += ins._setupCost;
				setup = false;
			}
		}
		else
		{
			setup = true;
		}
	}

	if (costCur != _prodCost)
		return false;


	return ok;
}


//compute _timeAtStation & _timeAtMP & _vehLoad
void Solution::computeTimeAndVehLoad(const Instance& ins)
{
	_timeAtStation.assign(_nbStation + 2, 0);
	_timeAtStation[0] = 0; //begin the tour at date 0

	_vehLoad.push_back({ 0, ins._vehH0 });
	int lastLoad = ins._vehH0;
	
	int j = 0;

	while (j <= _nbStation)
	{
		if (!_refuelSt[j])
		{
			//go straight from j to j+1
			_timeAtStation[j + 1] = _timeAtStation[j] + ins._time[j];
			_vehLoad.push_back({ _timeAtStation[j + 1], lastLoad - ins._energy[j]});
			lastLoad = lastLoad - ins._energy[j];
		}
		else
		{
			//find the refuelling correspondant period
			int i = refuelStToPeriod(j);

			//veh arrival at j+1 = (end of refuelling) + time from MP to j+1
			_timeAtStation[j + 1] = (i+1)*ins._p + ins._timeFromMP[j+1];

			//we know when the vehicle arrives and leaves the micro plant:
			int arrivalTime = _timeAtStation[j] + ins._timeToMP[j];
			_timeAtMP.push_back({ arrivalTime, (i + 1) * ins._p });

			//add the dates of arrival at the MP, beginning of refuelling (if != from arrival) and end of refueling
			// 1. arrival at the MP
			_vehLoad.push_back({ arrivalTime, lastLoad - ins._energyToMP[j] });
			lastLoad = lastLoad - ins._energyToMP[j];

			//2.  beginning of refuelling
			if (i * ins._p != arrivalTime)
				_vehLoad.push_back({ i * ins._p, lastLoad });

			//3. end of refuelling
			_vehLoad.push_back({ (i+1) * ins._p, lastLoad + _quantity[j] });
			lastLoad = lastLoad + _quantity[j];

			//4. next station (just after refuelling)
			_vehLoad.push_back({ _timeAtStation[j + 1], lastLoad - ins._energyFromMP[j + 1] });
			lastLoad = lastLoad - ins._energyFromMP[j + 1];

		}
		j++;
	}
}



//return the period index during which the vehicle refuels when it refuels after station j
int Solution::refuelStToPeriod(int j)
{
	int i = 0;

	//we suppose that the _synch matrix & j are correct: there is one value such that _synch[i][j] = true
	while (!_synch[i][j])
		i++;

	return i;
}


//compute _MPLoad & _vehLoad 
void Solution::computeMPLoad(const Instance& ins)
{
	_MPLoad.resize(ins._nbPeriod+1);

	_MPLoad[0] = ins._MPH0;
	
	//MPLoad[_nbPeriod] gives the MP load at the end of the time horizon
	for (int i = 1; i <= _nbPeriod; ++i)
	{
		// _MPLoad[i] = load at the beginning of period i (= end of period i-1)
		if (_produce[i - 1])
			_MPLoad[i] = _MPLoad[i - 1] + ins._prodRate[i - 1];
		else
		{
			//no production in i-1, the vehicle maybe refuels
			_MPLoad[i] = _MPLoad[i - 1];
			_MPLoad[i] -= _refuelP[i - 1];
		}
	}
}

//return the total H2 needed by the vehicle (_refuelSt must be up to date)
int Solution::sumRefuel()
{
	return accumulate(_quantity.begin(), _quantity.end(), 0);
}



//return the next station (after j) after which the vehicle refuels
//return M+1 if j is the latest station after which the vehicle refuels
int Solution::nextRefuelStation(int j)
{
	int next = j + 1;
	while (next <= _nbStation && !_refuelSt[next])
		next++;

	return next;
}

//returns the date (in time units, not in period) at which the vehicle begins to refuel after visiting station j
// _synch must be up to date: i.e. contains one i such that _synch[i][j] = true
int Solution::beginRefuel(const Instance& ins, int j)
{
	int i = 0;

	//we suppose that the vehicle refuels after j (no need to check i < nbPeriod)
	while (!_synch[i][j])
		i++;

	return i * ins._p ;
}



//ok = true if the solution is completely builded and valid (false if the algo did not succeed to build it)
//insName is the instance name, used to create the file name to save the solution
void Solution::enregistreSol(const Instance& ins, string insName, bool ok, double cpuTime)
{
	string name = "sol_" + insName + ".csv";
	ofstream fic(name, ios::out);

	//open an other file to store the details about the refuellings (this file will be used by the MIP)
	string name2 = "solStation_" + insName + ".txt";
	ofstream fic2(name2, ios::out);

	fic << insName << ";";

	//if the solution exists, fill the file
	if (ok)
	{
		fic << _totalTime << ";" << _prodCost << ";" << _totalTime + _prodCost << ";"
			<< cpuTime << ";" << computeNbRefuel() << ";" << computeNbSetup() << ";" << computeWaitTime(ins)  << endl;

		fic2 << _nbStation << endl;
		for (int j = 0; j <= _nbStation; ++j)
		{
			fic2 << _refuelSt[j] << " ";
		}
		fic2 << endl;
	}

	fic.close();
	fic2.close();
}



//======================= compute global information about the solution ==========================
int Solution::computeNbRefuel()
{
	int cpt = 0;
	for (const bool& b : _refuelSt)
	{
		if (b)
			cpt++;
	}

	return cpt;
}

int Solution::computeNbSetup()
{
	int cpt = 0;
	bool setup = true;    //true if we have to count a setup for the next production period

	for (const bool& b : _produce)
	{
		if (b) //we produce
		{
			if (setup) 
				cpt++;
			setup = false; //setup false for the next period
		}
		else
			setup = true; //we do not produce => setup true for the next prod period
	}

	return cpt;
}

int Solution::computeWaitTime(const Instance & ins)
{
	int timeCur = 0; //arrival time at station j when enter the loop in j

	for (int j = 0; j <= _nbStation; ++j)
	{
		//go straight from j to j+1
		if (!_refuelSt[j])
		{
			timeCur += ins._time[j];       //arrival time at station j+1
			
		}
		else //j -> MP -> j+1
		{
			timeCur += ins._timeToMP[j];
			timeCur += ins._p; //refuel
			timeCur += ins._timeFromMP[j + 1]; //go from MP to j+1
		}
	}

	//here timeCur = time at final depot without the waiting times
	//hence wait time = total travel time - timeCur  

	return _totalTime - timeCur;
}
