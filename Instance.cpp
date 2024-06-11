#include "Instance.h"
#include <numeric>


//read an instance contained in 2 data files 
void Instance::readDataFile(const string& name1, const string& name2)
{
	cout << "lecture " << name1 << " " << name2 << endl;

	ifstream file1(name1);   // all data except production
	ifstream file2(name2);   // production data

	assert(file1);
	assert(file2);

	int useless1, useless2;  // 2 data, useless for our algorithm but present in the data file

	//================== READ DATA FROM FILE 1 =========================

	file1 >> _nbStation >> _Tmax >> _vehH0 >> _capVeh >> _p >> useless1 >> useless2 >> _setupCost >> _MPH0 >> _capMP;

	string s;
	file1 >> s; //pass the string "stations" 

	// allocate the stations array coord (put the micro-plant coordinate at index nbStation+1)
	_coord.resize(_nbStation + 3);

	//read the coordinates of the stations (including the depot = station 0)
	for (int i = 0; i <= _nbStation; ++i)
	{
		int x, y;
		file1 >> x >> y;
		_coord[i] = { x, y };
	}

	//final depot
	_coord[_nbStation + 1] = _coord[0];

	//add the micro plant coordinate = depot coordinate + (0,1)
	_coord[_nbStation + 2] = _coord[0];
	_coord[_nbStation + 2].second++;

	//================== READ DATA FROM FILE 2 (production) =========================

	// we need first to compute the number of period

	_nbPeriod = _Tmax / _p;
	if (_Tmax % _p != 0)
		_nbPeriod++;

	// allocate the production arrays
	_prodRate.resize(_nbPeriod);
	_prodCost.resize(_nbPeriod);

	// we now read the data
	for (int j = 0; j < _nbPeriod; ++j)
		file2 >> _prodRate[j];
	
	for (int j = 0; j < _nbPeriod; ++j)
		file2 >> _prodCost[j];

	file1.close();
	file2.close();
}



//preprocess the data read to complete the instance data
void Instance::preprocess()
{

	assert(_nbStation > 0);

	// allocate the arrays

	_time.resize(_nbStation + 1);                       // _time[j] = time to go from j to j+1 ( j = 0...nbStation) [-> t]
	_timeToMP.resize(_nbStation + 1);                   // _timeToMP[j] = time to go from j to micro plant ( j = 0...nbStation) [-> d]
	_timeFromMP.resize(_nbStation + 2);                 // _timeFromMP[j] = time to go from micro plant to j ( j = 0...nbStation+1) [-> d*]
	_energy.resize(_nbStation + 1);                     // _energy[j] = energy to go from j to j+1 ( j = 0...nbStation) [-> e]
	_energyToMP.resize(_nbStation + 2);                 // _energyToMP[j] = energy to go from j to micro plant ( j = 0...nbStation) (we add a dummy value for j = nbStation+1, usefull in the algo) [-> eps]
	_energyFromMP.resize(_nbStation + 2);               // _energyFromMP[j] = energy to go from micro plant to j ( j = 0...nbStation+1) [-> eps*]


	// fill the array

	//micro plant coordinates
	auto [xMP, yMP] = _coord[_nbStation + 2];

	for (int i = 0; i <= _nbStation; ++i)
	{
		auto [x1, y1] = _coord[i];
		auto [x2, y2] = _coord[i+1];

		_time[i] = euclidianDistance(x1, y1, x2, y2);
		_timeToMP[i] = euclidianDistance(x1, y1, xMP, yMP);
		_timeFromMP[i] = euclidianDistance(xMP, yMP, x1, y1);

		_energy[i] = manhattanDistance(x1, y1, x2, y2);
		_energyToMP[i] = manhattanDistance(x1, y1, xMP, yMP);
		_energyFromMP[i] = manhattanDistance(xMP, yMP, x1, y1);
	}

	auto [x1, y1] = _coord[0];
	_timeFromMP[_nbStation + 1] = euclidianDistance(xMP, yMP, x1, y1);
	_energyFromMP[_nbStation + 1] = manhattanDistance(xMP, yMP, x1, y1);
	_energyToMP[_nbStation + 1] = _vehH0; //(dummy value to avoid a particular case in the algo)
}



//display the instance in text format
void Instance::display()
{

	cout << "---------------------  INSTANCE DATA -----------------------------" << endl << endl;

	cout << "STATIONS and VEHICLE DATA" << endl << endl;

	cout << "TMAX = " << _Tmax << endl;
	cout << _nbStation << " stations" << endl;
	cout << _nbPeriod << " periods of duration " << _p << endl;

	cout << "vehicle init load = " << _vehH0 << " ; max load = " << _capVeh << endl;
	cout << "MP init load = " << _MPH0 << " ; max load = " << _capMP << endl << endl;

	cout << "time:           "; displayVector(cout, _time);
	cout << "time to MP:     "; displayVector(cout, _timeToMP);
	cout << "time from MP:   "; displayVector(cout, _timeFromMP);
	cout << "energy:         "; displayVector(cout, _energy);
	cout << "energy to MP:   "; displayVector(cout, _energyToMP);
	cout << "energy from MP: "; displayVector(cout, _energyFromMP);


	cout << endl;
	cout << "PRODUCTION DATA" << endl << endl;

	cout << "setupCost = " << _setupCost << endl;

	cout << "production rate:           "; displayVector(cout, _prodRate);
	cout << "production cost:           "; displayVector(cout, _prodCost);

	cout << "--------------------------------------------------------------------" << endl;
}


//draw the instance using Python
void Instance::draw(const string & name)
{
	//1. first export the data for python 

	ofstream fic(name + "_py.txt");

	fic << _nbPeriod << endl;
	fic << _p << endl;
	fic << _nbStation << endl;

	displayVector(fic, _prodRate);
	displayVector(fic, _prodCost);

	displayVector(fic, _time);
	displayVector(fic, _energy);
	displayVector(fic, _timeToMP);
	displayVector(fic, _energyToMP);

	fic.close();

	// 2. second call python 
	string cmd = "python drawStations.py " + name + "_py";
	//system(cmd.c_str());

}


// return the total amount of H2 the micro plant can produce (if it produces no stop from period 0 to period N-1)
int Instance::prodCapacity() const
{
	return accumulate(_prodRate.begin(), _prodRate.end(), 0);
}


//returns the time to go from MP to station next to station next + 1 ... to last 
// i.e. time (MP -> next -> next + 1 -> ... -> last -> MP)
int Instance::timeSubTour(int next, int last) const
{
	int timeTour = _timeFromMP[next];
	int j = next;

	while (j < last)
	{
		timeTour += _time[j];
		j++;
	}

	timeTour += _timeToMP[last];

	//cout << "time subtour MP -> " << next << "-> ... -> " << last << "-> MP = " << timeTour << endl;

	return timeTour;
}

//returns the time to go from depot to MP after station last 
// i.e. time (depot -> 1 -> 2 -> ... -> last -> MP)
int Instance::timeSubTourInit(int last) const
{
	int timeTour = 0;
	int j = 0;

	while (j < last)
	{
		timeTour += _time[j];
		j++;
	}

	timeTour += _timeToMP[last];

	return timeTour;
}

//returns the time to go from MP to last to final depot 
// i.e. time (MP -> last -> .. -> final depot)
int Instance::timeSubTourFinal(int last) const
{
	int timeTour = _timeFromMP[last];
	int j = last;

	while (j < _nbStation+1)
	{
		timeTour += _time[j];
		j++;
	}

	return timeTour;
}

//=====================================================================
// auxiliary functions


int euclidianDistance(int x1, int y1, int x2, int y2)
{
	double tmp = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return static_cast<int> (ceil(tmp) + 0.00001);
}


int manhattanDistance(int x1, int y1, int x2, int y2)
{
	int d1 = static_cast<int> (abs(x2 - x1));
	int d2 = static_cast<int> (abs(y2 - y1));
	return d1 + d2;
}

// display each element of a integer vector in a line
void displayVector(ostream & os, vector<int>& v)
{
	for (int i : v)
		os << i << " ";
	os << endl;
}

// display each element of a integer vector in a line
void displayVector(ostream& os, vector<pair<int, int> >& v)
{
	for (const pair<int,int> & p : v)
		os << p.first << " " << p.second << " ";
	os << endl;
}

