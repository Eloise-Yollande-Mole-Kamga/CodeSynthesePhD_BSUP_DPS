#pragma once


#include "Solution.h"
#include "Instance.h"

#include "util.h"


// this file implement a 2-steps heuristic to solve the SMEPC:
// it first computes after which stations the vehicle should refuel using a shortest path algorithm in a ad hoc graph
//    (without taking into account the production constraints)
// then it computes a production planning that is compliant with the vehicle schedule


struct Label
{
	int _pred = 0;
	int _cost = 0;
};


//associate a refuel station with a period and minimal quantity to refuel
struct InfoRefuel
{
	int _earliestPeriod;
	int _station;
	int _quantity;
};


//variable that keep a production strategy (consecutive prod. period)
struct ProdStrategie
{
	int _i1 = -1; //first prod period
	int _i2 = -1; //last prod period
	int _Q = 0;  //quantity produced (from i1 to i2)
	int _C;  //cost (included setup)
};


class Algo
{
private:

	const Instance * _ins; //pointer on the instance to solve

	vector < Label > _label; //_label[j] = (pred, cost) of node j where pred = predecessor in the shortest path to j and cost is the cost of this shortest path


	// data computed by computeEarliestRefPeriod
	vector<InfoRefuel> _info;	      //associate each refuel station with the earliest period and the minimal quantity to refuel
	int _margin;                      //margin (how many periods we can wait to refuel)



public:

	//initialize the data structures and the pointer on the instance
	void init(const Instance * ins);

	//main function: run the algorithm and build a Solution for the instance _ins
	//returns true if a solution has been found
	bool run(Solution& sol, int strategie = 1);

	// cette fonction calcule une solution heuristique pour le probleme de production
	// connaissant une solution au probleme "de refuel"
	// on a donc besoin des vecteurs qui decrive la solution du probleme de refuel :
	// ref(j) = true si on recharge apres la station j
	// qte(j) = quantite rechargee apres la station j (0 si pas de recharge)

	bool runProd(Solution& sol, int strategie, const vector<bool>& ref, const vector<int>& qte);

	//clear the data 
	void clear()
	{
		_label.clear();
		_info.clear();
		_label.resize(_ins->_nbStation + 2);
	}


private:



	// fill the _label DS (data structure) using a "shortest path algorithm" in an implicit graph
	void computeLabel();

	// fill the stations indexed array of Solution given in parameter with the DS _label
	// (NB. the label does not give any information about the production part of the solution neither synchronization)
	// return true if the instance is feasible, false otherwise
	bool fillSolFromLabel(Solution & sol);

	//fill the refueling part of the Solution with the vectors in parameter
	void fillRefuelSolFromVectors(Solution& sol, const vector<bool>& ref, const vector<int>& qte);

	// compute the production schedule from the vehicle schedule 
	// (i.e. from the _refuelSt & _quantity vectors)
	// return true if a solution is found
	bool computeProd_strategie1(Solution& sol);


	//===============================================
	// 
	// 
	// compute the production schedule from the vehicle schedule 
	// (i.e. from the _refuelSt & _quantity vectors)
	// return true if a solution is found
	bool computeProd_strategie2(Solution& sol);

	void computeEarliestRefPeriod(Solution& sol);

	//returns when we should start to produce to produce at least Q units between periods i1 and i2 (included)
	// we consider only consecutive periods of production
	// if we failed to produce QToProduce, we return the best we did
	//return (first period of production, last period of production, quantity produced, cost)
	ProdStrategie  computeProdBetweenPeriod(int i1, int i2, int QToProduce, int Qinit);

	//returns the best sub period to produce (we consider only consecutive periods to produce)
	// and we keep the consecutive periods which give us the most H2 at the lowest price 
	//return (first period of production, last period of production, quantity produced, cost)
	ProdStrategie computeBestProdBetweenPeriod(int i1, int i2, int Qinit);
};


//====================================================================================
// fonction qui "fait tout" : prend en entrée le fichier et 
// retourne une solution (si trouvee, -1 sinon) du probleme global (recharge + production)
// pour utiliser comme borne sup dans le DPS global
//====================================================================================

//lit les fichiers et resout le probleme global (refuel + prod) avec les heuristiques, retourne la meilleure solution
int calcul_BS_heuristique(const string & nomFic, const string& nomFicProd);


//lit les fichiers et resout le probleme de production connaissant la solution du refuel passee en parametre :
// ref(j) = true si on recharge apres la station j
// qte(j) = quantite rechargee apres la station j (0 si pas de recharge)
int calcul_BS_Prod_heuristique(const string& nomFic, const string& nomFicProd, const vector<bool> & ref, const vector<int> & qte);