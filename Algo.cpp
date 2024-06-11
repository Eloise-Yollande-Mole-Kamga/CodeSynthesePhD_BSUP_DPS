#include "Algo.h"
#include <algorithm>
#include <cmath>

//====================================================================================
// fonction qui "fait tout" : prend en entrée le fichier et 
// retourne une solution (si trouvee, -1 sinon) du probleme global (recharge + production)
// pour utiliser comme borne sup dans le DPS global
//====================================================================================

// nomFic et nomFicProd sont les chemins complets vers les fichiers a lire
int calcul_BS_heuristique(const string & nomFic, const string & nomFicProd)
{
	int resultat = -1;

	//-------------------------------------------------------------
	// 1. create an instance from data file and display it

	Instance ins;

	//read the data files
	ins.readDataFile(nomFic, nomFicProd);

	//preprocess to data in order to obtain the time and energy
	ins.preprocess();

	//display in text format the instance
	//ins.display();

	//-------------------------------------------------------------
	// 2. compute a solution, check its validity and display

	//  compute automatically with the algorithm of project step 2

	// create an algo object
	Algo algo;
	algo.init(&ins);

	// ---  run the first version of the algorithm  ---
	Solution sol;

	bool ok = algo.run(sol);
	if (ok)
	{
		if (!sol.checkValidity(ins))
		{
			cout << "calcul_BS_heuristique : solution non valide" << endl;
			exit(-1);
		}
		else
		{
			//cout << "SOLUTION OK" << endl;
			//sol.display();
			resultat = sol._prodCost + sol._totalTime;
		}
	}


	// ---  run the second version of the algorithm  ---
	Solution sol2;
	algo.clear();
	ok = algo.run(sol2, 2);
	
	if (ok)
	{
		if (!sol2.checkValidity(ins))
		{
			cout << "calcul_BS_heuristique : solution 2 non valide" << endl;
			exit(-1);
		}
		else
		{
			//cout << "SOLUTION OK" << endl;
			//sol.display();
			if (sol2._prodCost + sol2._totalTime < resultat)
				resultat = sol2._prodCost + sol2._totalTime;
		}
	}

	return resultat;
}

//lit les fichiers et resout le probleme de production connaissant la solution du refuel passee en parametre :
// ref(j) = true si on recharge apres la station j
// qte(j) = quantite rechargee apres la station j (0 si pas de recharge)
int calcul_BS_Prod_heuristique(const string& nomFic, const string& nomFicProd, const vector<bool>& ref, const vector<int>& qte)
{
	int resultat = -1;

	//-------------------------------------------------------------
	// 1. create an instance from data file and display it

	Instance ins;

	//read the data files
	ins.readDataFile(nomFic, nomFicProd);

	//preprocess to data in order to obtain the time and energy
	ins.preprocess();

	//display in text format the instance
	//ins.display();

	//-------------------------------------------------------------
	// 2. compute a solution, check its validity and display

	//  compute automatically with the algorithm of project step 2

	// create an algo object
	Algo algo;
	algo.init(&ins);

	// ---  run the first version of the algorithm  ---
	Solution sol;

	bool ok = algo.runProd(sol, 1, ref, qte);
	if (ok)
	{
		if (!sol.checkValidity(ins))
		{
			cout << "calcul_BS_heuristique : solution non valide" << endl;
			exit(-1);
		}
		else
		{
			//cout << "SOLUTION OK" << endl;
			//sol.display();
			resultat = sol._prodCost;
		}
	}


	// ---  run the second version of the algorithm  ---
	Solution sol2;
	algo.clear();
	ok = algo.runProd(sol2, 2, ref, qte);

	if (ok)
	{
		if (!sol2.checkValidity(ins))
		{
			cout << "calcul_BS_heuristique : solution 2 non valide" << endl;
			exit(-1);
		}
		else
		{
			//cout << "SOLUTION OK" << endl;
			//sol.display();
			if (sol2._prodCost < resultat)
				resultat = sol2._prodCost;
		}
	}

	return resultat;
}


//====================== fonction de la classe Algo =================


//initialize the data structures and the pointer on the instance
void Algo::init(const Instance * ins)
{
	_ins = ins;

	// complete with the initialization of the data structures needed for the algorithm
	_label.resize(ins->_nbStation+2);
}



//main function: run the algorithm and build a Solution for the instance _ins
//returns true if a solution has been found
bool Algo::run(Solution & sol, int strategie)
{
	bool ok = true;

	//0. initialisation
	sol.init(_ins->_nbStation, _ins->_nbPeriod);

	//1. compute the optimal refuelling stations for the vehicle
	computeLabel();
	ok = fillSolFromLabel(sol);

	//2. compute a production schedule compatible with the vehicle refuelling schedule
	if (ok)
	{	

		switch (strategie)
		{
		case 1:
			//strategie 1 : on produit le plus tot possible
			ok = computeProd_strategie1(sol);
			break;

		case 2:
			//strategie 2 : 
			ok = computeProd_strategie2(sol);
			break;

		}


		//2.1. if a solution is found, compute the associated dates and load (in order to draw and check the solution)
		if (ok)
		{
			sol.computeTimeAndVehLoad(*_ins);
			sol.computeMPLoad(*_ins);
		}
	}


	return ok;
}

// cette fonction calcule une solution heuristique pour le probleme de production
// connaissant une solution au probleme "de refuel"
// on a donc besoin des vecteurs qui decrive la solution du probleme de refuel :
// ref(j) = true si on recharge apres la station j
// qte(j) = quantite rechargee apres la station j (0 si pas de recharge)

bool Algo::runProd(Solution& sol, int strategie, const vector<bool> & ref, const vector<int> & qte)
{
	bool ok = true;

	//0. initialisation
	sol.init(_ins->_nbStation, _ins->_nbPeriod);

	//1. fill the refueling solution part with the vectors given as parameter
	fillRefuelSolFromVectors(sol, ref, qte);

	//2. compute a production schedule compatible with the vehicle refuelling schedule
	if (ok)
	{
		switch (strategie)
		{
		case 1:
			//strategie 1 : on produit le plus tot possible
			ok = computeProd_strategie1(sol);
			break;

		case 2:
			//strategie 2 : 
			ok = computeProd_strategie2(sol);
			break;
		}


		//2.1. if a solution is found, compute the associated dates and load (in order to draw and check the solution)
		if (ok)
		{
			sol.computeTimeAndVehLoad(*_ins);
			sol.computeMPLoad(*_ins);
		}
	}


	return ok;
}

// fill the _label DS using a "shortest path algorithm" in an implicit graph
void Algo::computeLabel()
{
	int M = _ins->_nbStation;

	//----------------------------------------------------
	// preprocess : compute the detours

	vector<int> detour(M + 1);
	for (int i = 0; i <= M; ++i)
		detour[i] = _ins->_p + _ins->_timeToMP[i] + _ins->_timeFromMP[i + 1] - _ins->_time[i];

	//----------------------------------------------------
	// label initialization
	_label[0] = { -1,0 };
	for (int j = 1; j <= M + 1; ++j)
	{
		_label[j] = { -1, 10 * _ins->_Tmax };
	}

	//compute the labels related to type 2 arcs
	int j = 1;
	int energySum = _ins->_energy[0] + _ins->_energyToMP[j];

	while (j <= M && energySum <= _ins->_vehH0)
	{
		_label[j] = { 0, 0 };
		j++;

		//update energySum = energy to perform the tour (Depot, station 1, ..., station j, micro-plant)
		energySum += _ins->_energy[j - 1] - _ins->_energyToMP[j - 1] + _ins->_energyToMP[j];
	}


	//compute the labels related to type 1 & 3 arcs
	for (int i = 0; i <= M; ++i)
	{
		//int costCur = _label[i]._cost;
		j = i + 1;
		energySum = _ins->_energyFromMP[i + 1] + _ins->_energyToMP[i + 1];

		while (j <= M + 1 && energySum <= _ins->_capVeh)
		{
			if (_label[j]._cost > _label[i]._cost + detour[i])
				_label[j] = { i, _label[i]._cost + detour[i] };
			j++;

			if (j <= M + 1)
				energySum += _ins->_energyToMP[j] - _ins->_energyToMP[j - 1] + _ins->_energy[j - 1];
		}
	}
}



//fill the Solution given in parameter with the DS _label
bool Algo::fillSolFromLabel(Solution& sol)
{
	bool ok = true;

	assert(sol._nbStation > 0);
	assert(_label.size() > 0);

	int M = _ins->_nbStation;
	vector<int> stRefuel; //list of stations after which the vehicle refuels (in ascending order)

	sol._refuelSt.assign(M + 1, false);


	// 1. follow the predecessor to know when the vehicle refuels
	int cur = _label[M + 1]._pred;
	int suiv = M + 1;

	if (cur == -1)
	{
		cout << "instance not feasible" << endl;
		ok = false;
	}
	else
	{
		//cout << "LABEL M+1 COST = " << _label[M + 1]._cost << endl;

		do
		{
			sol._refuelSt[cur] = true;
			stRefuel.push_back(cur);
			suiv = cur;
			cur = _label[cur]._pred;
		} while (cur > 0);

		//particular case with depot: we must the initial energy load to know if the vehicle refuels just after depot or not
		// (2 differents arc types come from depot: see the modelization)

		int sumEnergy = _ins->_energyToMP[suiv];
		for (int k = 0; k < suiv; k++)
			sumEnergy += _ins->_energy[k];

		if (sumEnergy > _ins->_vehH0)
		{
			sol._refuelSt[0] = true;
			stRefuel.push_back(0);
		}


		//reverse stRefuel to have stations in ascending order
		reverse(stRefuel.begin(), stRefuel.end());

		//add the final depot to avoid the particular case of coming back to depot
		stRefuel.push_back(M + 1);

		//2. compute the minimal H2 units needed by the vehicle at each refuelling
		sol._quantity.assign(M + 1, 0);
		int s = static_cast<int>(stRefuel.size()) - 1;

		//current hydrogen load in the vehicle tank, we update this quantity at each refuelling
		int currentLoad = _ins->_vehH0;

		//compute the H2 quantity in the veh tank when it arrives at the first refuelling
		int first = stRefuel[0];
		for (int l = 0; l < first; ++l)
			currentLoad -= _ins->_energy[l];

		currentLoad -= _ins->_energyToMP[first];


		//compute the quantity to refuel at each refuelling transaction
		// we suppose that we refuel the minimal quantity to be able to go to the next refuelling (or final depot)
		for (int k = 0; k < s; ++k)
		{
			int i = stRefuel[k];
			int j = stRefuel[k + 1];

			//refuel after station i, then after station j : 
			// sol._quantity[i] = energy to do the path MP -> i+1 -> ... -> j -> MP
			sol._quantity[i] = _ins->_energyFromMP[i + 1] + _ins->_energyToMP[j];

			for (int l = i + 1; l <= j - 1; ++l)
				sol._quantity[i] += _ins->_energy[l];

			sol._quantity[i] -= currentLoad; //for the first refuelling currentLoad may be > 0
			currentLoad = 0; //we suppose that we refuel the minimal quantity so currentLoad is 0 for all refuelling except the first one

			//since it is faster to go straight, the label algorithm should not ask us to refuel if we do not really need it
			assert(sol._quantity[i] > 0);
		}
	}

	return ok;
}

//fill the refueling part of the Solution with the vectors in parameter
void Algo::fillRefuelSolFromVectors(Solution& sol, const vector<bool>& ref, const vector<int>& qte)
{
	//bool ok = true;

	assert(sol._nbStation > 0);

	//ces vecteurs sont supposes etre de taille nbStation + 1
	sol._refuelSt = ref;
	sol._quantity = qte;		
}

// compute the production schedule from the vehicle schedule 
// (i.e. from the _refuelSt & _quantity vectors)
// return true if a solution is found
// the production is scheduled as early as possible
bool Algo::computeProd_strategie1(Solution& sol)
{
	bool ok = true;
	sol._prodCost = 0;

	// 0. array initialisation for the production part & synchro
	sol._produce.assign(sol._nbPeriod, false);
	sol._refuelP.assign(sol._nbPeriod, 0);
	sol._synch.assign(sol._nbPeriod, vector<bool>(sol._nbStation+1, false)); 

	// 1. data initialisation
	int nextStation = sol.nextRefuelStation(-1); // next station after which we refuel
	int nextPeriod = 0;							 // period index such that the next refuel can not be before this period (i.e. the vehicle can not arrives at the MP before this period)
	int predStation = 0;                         // last station (before nextStation) after which the vehicle refueled 
	int mpLoadCur = _ins->_MPH0;				 // current H2 load in the MP tank
	int i = 0;									 // current period
	int sumRefuel = sol.sumRefuel();			 // total H2 the MP has to provide to the vehicle
	int totalProd = 0;							 // total quantity of H2 produced by the MP
	int lastRefuelP = 0;						 // last period where the vehicle refueled (used to compute the total tour time)
	bool setup = true;

	//first period from which the vehicle can refuel
	nextPeriod += static_cast<int>(ceil(static_cast<double>(_ins->timeSubTourInit(nextStation)) / _ins->_p) + 0.01);

	 
	// if the vehicle needs more than the MP can produce, there is no solution
	if (sumRefuel > _ins->prodCapacity())
		ok = false;
	else
	{
		// 2. Algorithm main idea: produce as soon as possible, stop when the vehicle refuels 
		// EXITING CONDITIONS:
		//    (nextStation = _ins->_nbStation+1) means the vehicle succeeded to refuel 
		//    (totalProd >= sumRefuel) means the MP produced enough to end with at least its initial level of hydrogen
		while ( i < _ins->_nbPeriod && (nextStation <= _ins->_nbStation || totalProd < sumRefuel))
		{
			while (i < _ins->_nbPeriod && i < nextPeriod)
			{
				// if there is enough empty space in the MP & we have not reached the total amount of H2 needed, we produce at period i
				if (mpLoadCur + _ins->_prodRate[i] <= _ins->_capMP && totalProd < sumRefuel)
				{
					sol._produce[i] = true;
					sol._prodCost += _ins->_prodCost[i];
					if (setup)
					{
						sol._prodCost += _ins->_setupCost;
						setup = false;
					}

					totalProd += _ins->_prodRate[i];
					mpLoadCur += _ins->_prodRate[i];

				}
				else
					setup = true;
				++i;
			}
			
			// if there is enough H2 in the MP tank, the vehicle refuels at period i
			if (i < _ins->_nbPeriod && mpLoadCur >= sol._quantity[nextStation])
			{
				setup = true;

				sol._refuelP[i] = sol._quantity[nextStation];
				mpLoadCur -= sol._quantity[nextStation];
				sol._synch[i][nextStation] = true;

				lastRefuelP = i;
				i++; //next possible period to produce

				//update the data for the next refuel (if any)
				predStation = nextStation;
				nextStation = sol.nextRefuelStation(nextStation);


				//if the tour is not done: nextPeriod = current one + 1 (time to refuel) + number periods for the vehicle come back at the MP while following its tour
				if (nextStation <= _ins->_nbStation)
					nextPeriod += 1 + static_cast<int>(ceil(static_cast<double>(_ins->timeSubTour(predStation + 1, nextStation)) / _ins->_p) + 0.01);
				else //the tour is done, but the MP maybe need to keep on the production to come back to its initial level
					nextPeriod = _ins->_nbPeriod;
				
			}
			else //we wait one more period to refuel
			{
				nextPeriod++;
			}


		}//end main while

		//if we do not reach the final depot or we do not produce enough => we did not succeed building a solution
		if (nextStation <= _ins->_nbStation || totalProd < sumRefuel)
			ok = false;
	}

	//compute the date when the vehicle comes back to depot (if sol ok)
	if (ok)
	{
		//total time = date of arrival at last refuel + time refuel + time to come back
		sol._totalTime = lastRefuelP * _ins->_p + _ins->_p + _ins->timeSubTourFinal(predStation+1);
	}

	return ok;
}


// compute the production schedule from the vehicle schedule 
// (i.e. from the _refuelSt & _quantity vectors)
// return true if a solution is found
bool Algo::computeProd_strategie2(Solution& sol)
{

	bool ok = true;
	int M = _ins->_nbStation;
	int offset = 0; //nb de periodes duquel on retarde les recharges (dans le cas ou on n'a pas assez de H2 pour le refuel)

	computeEarliestRefPeriod(sol);
	int sumRefuel = sol.sumRefuel();//remaining quantity to refuel, thus to produce
	sol._prodCost = 0;


	int S = static_cast<int>(_info.size());
	int pPrec = -1;//last refuel period 
	int citCur = _ins->_MPH0;

	//for each refuel transaction we check if we have enough H2

	for (int cpt = 0; cpt < S; ++cpt)
	{
		int pCur = _info[cpt]._earliestPeriod; //current refuel period
		int Qneeded = _info[cpt]._quantity;
		
		//-------------------------------------------------
		// 1. find the best production between two refuels
		// 
		
		//if we still need lots of H2, we compute how to produce the more at the lowest price
		ProdStrategie prodSt;
		if (sumRefuel + citCur > _ins->_capMP)
			prodSt = computeBestProdBetweenPeriod(pPrec + 1, pCur - 1 + offset, citCur);
		else //we produce what we need, no more
		{
			if (sumRefuel > 0)
				prodSt = computeProdBetweenPeriod(pPrec + 1, pCur - 1 + offset, sumRefuel, citCur);
		}

		//in case of failure we try to make the vehicle wait and produce only what we need for the next refueling
		while (prodSt._Q + citCur < Qneeded && _margin > 0)
		{
			_margin--;
			offset++;
			
			prodSt = computeProdBetweenPeriod(pPrec + 1, pCur - 1 + offset, Qneeded - citCur, citCur);
		}

		//if margin becomes < 0, we failed
		if (_margin < 0)
			return false;

		//-------------------------------------------------
		// 2. plan the production (if any)
		if (prodSt._i1 >= 0)
		{
			for (int i = prodSt._i1; i <= prodSt._i2; ++i)
			{
				sol._produce[i] = true;
				sumRefuel -= _ins->_prodRate[i];
				citCur += _ins->_prodRate[i];
			}

			sol._prodCost += prodSt._C;
		}

		//update the solution wrt the refuel
		pPrec = _info[cpt]._earliestPeriod + offset; //period when the vehicule will refuel
		sol._refuelP[pPrec] = _info[cpt]._quantity;
		sol._synch[pPrec][_info[cpt]._station] = true;

		citCur -= _info[cpt]._quantity;
		
	}

	//----------------- compute the total time for the vehicule to finish the tour --------------
	sol._totalTime = pPrec * _ins->_p + _ins->_p; //end of last refuel
	sol._totalTime += _ins->_timeFromMP[_info[S - 1]._station + 1]; //come back to the tour
	for (int j = _info[S - 1]._station + 1; j <= M ; ++j) //finish the tour
		sol._totalTime += _ins->_time[j];

	//we may need to produce again to finish with the initial amount of H2 in the tank
	if (sumRefuel > 0)
	{
		ProdStrategie prodSt = computeProdBetweenPeriod(pPrec + 1, _ins->_nbPeriod-1, sumRefuel, citCur);
		if (prodSt._Q < sumRefuel)
			return false; //fail

		//in case of succeess we just need to update the production
		sol._prodCost += prodSt._C;
		for (int i = prodSt._i1; i <= prodSt._i2; ++i)
			sol._produce[i] = true;
		
	}

	return ok;
}


void Algo::computeEarliestRefPeriod(Solution& sol)
{
	//bool ok = true;
	int M = _ins->_nbStation;

	// 1. we compute when we can refuel (earliest)

	int j = 0;
	double cumulTime = 0;
	while (j <= M)
	{
		while (j <= M && !sol._refuelSt[j])
		{
			cumulTime += _ins->_time[j];
			j++;
		}
		if (j <= M && sol._refuelSt[j])
		{
			cumulTime += _ins->_timeToMP[j];
			int startRefuel = static_cast<int>(ceil(cumulTime / _ins->_p) + EPSILON);

			_info.push_back({startRefuel, j, sol._quantity[j]});

			//date when the vehicle is at the next station (after refuel)
			cumulTime = (startRefuel + 1) * _ins->_p + _ins->_timeFromMP[j + 1];
			j++;
		}
	}

	//if (cumulTime != sol._totalTime)
	//	stopProgram("Algo::computeEarliestRefPeriod : cumulTime != sol._totalTime");

	
	_margin = static_cast<int>(floor((_ins->_Tmax - cumulTime) / _ins->_p));

}

//returns when we should start to produce to produce at least Q units between periods i1 and i2 (included)
// if we failed to produce QToProduce, we return the best we did
// we consider only consecutive periods of production
//return (first period of production, last period of production, quantity produced, cost)
ProdStrategie Algo::computeProdBetweenPeriod(int i1, int i2, int QToProduce, int Qinit)
{
	
	int i1Cur = i1;

	int best_i1 = -1, best_i2 = -1, bestQ = -1, bestCost = INFINI_LN;

	//be careful to have enough place to produce what we want to
	if (QToProduce + Qinit > _ins->_capMP)
		return{ -1,-1,-1,-1 };
	
	while (i1Cur <= i2)
	{
		int i = i1Cur;
		//begin the production in i
		int citCur = Qinit;
		int costCur = _ins->_setupCost;

		//consecutive production from i
		while (i <= i2 && citCur + _ins->_prodRate[i] <= _ins->_capMP)
		{
			citCur += _ins->_prodRate[i];
			costCur += _ins->_prodCost[i];
			i++;
		}

		//if I have produced enough
		if (citCur - Qinit >= QToProduce)
		{
			if (costCur < bestCost)
			{
				best_i1 = i1Cur;
				best_i2 = i - 1;
				bestQ = citCur - Qinit;
				bestCost = costCur;
			}
		}
		else//I did not produce enough, I keep the most I produced
		{
			if (citCur - Qinit > bestQ )
			{
				best_i1 = i1Cur;
				best_i2 = i - 1;
				bestQ = citCur - Qinit;
				bestCost = costCur;
			}
		}

		i1Cur++;
	}


	return{ best_i1, best_i2, bestQ, bestCost };

}




//returns the best sub period to produce (we consider only consecutive periods to produce)
// and we keep the consecutive periods which give us the most H2 at the lowest price 
//return (first period of production, last period of production, quantity produced, cost)
ProdStrategie Algo::computeBestProdBetweenPeriod(int i1, int i2, int Qinit)
{
	bool stop = false;
	int i1Cur = i1;

	int best_i1 = -1, best_i2 = -1, bestQ = -1, bestCost = INFINI_LN;


	while (i1Cur <= i2 && !stop)
	{
		int i = i1Cur;
		//begin the production in i
		int citCur = Qinit;
		int costCur = _ins->_setupCost;

		//consecutive production from i
		while (i <= i2 && citCur + _ins->_prodRate[i] <= _ins->_capMP)
		{
			citCur += _ins->_prodRate[i];
			costCur += _ins->_prodCost[i];
			i++;
		}

		//if the loop stops because the tank is full
		if (i <= i2)
		{
			double produced = citCur - Qinit;

			//we keep the lower price per unit
			if (produced/costCur < (double)(bestQ)/bestCost)
			{
				best_i1 = i1Cur;
				best_i2 = i - 1;
				bestQ = citCur - Qinit;
				bestCost = costCur;
			}
		}
		else
		{
			//we keep the current production if we have nothing else
			if (best_i1 == -1)
			{
				best_i1 = i1Cur;
				best_i2 = i - 1;
				bestQ = citCur - Qinit;
				bestCost = costCur;
				stop = true;
			}
		}


		i1Cur++;
	}


	return{ best_i1, best_i2, bestQ, bestCost };

}