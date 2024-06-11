#pragma once

#include <iostream>
#include <string>

using namespace std;

const double EPSILON = 0.000001;
const int INFINI_LN = 1000000;

inline void stopProgram(string msge, bool stop = true)
{
	cout << "ERREUR : ";
	cout << msge << endl;
	if (stop)
	{
#ifdef _WIN32
		int stop2;
		cin >> stop2;
#else
		exit(-1);
#endif
	}
}