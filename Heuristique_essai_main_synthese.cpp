//Programme dynamique global du SMEPC
#include <iostream>
#include <math.h>
#include <fstream>
#include <list>
#include <string>
#include <time.h>
#include<chrono>
//#include <algorithm>
//#include <vector>
//#include "Header.h"
//#include "HeaderdynGlobal.h"

//#include "HeaderPipeline.h"
//#include "HeaderProgDyn.h"
//#include "HeaderNS_ProgDyn.h"
//#include "Header_BSUP_prod_pipeline.h"

#include "Header_essai_main_synthese.h"

#define tmp_h 0 //la durée de la recharge est 1
using namespace std;
Heuristique::Heuristique()
{
	//ctor
}

Heuristique::~Heuristique()
{
	//dtor
}

/******************************Estimation recharge ***************************/
//cet algorithme permet d'avoir les bornes inférieurs pour l'énergie (utile pour le filtrage du programme globale)
void Heuristique::dyn_Recharge_energie() //[p OK]
{
	int alpha = 1;
	int beta = 0;

	std::list <Etat_Recharg> list_recharge;
	std::list<Etat_Recharg>::iterator it;

	REC[Nb_stations + 1] = 0;
	REC_[Nb_stations + 1][0].V = v0;//énergie dans le véhicule en Nb_stations + 1
	REC_[Nb_stations + 1][0].T = 0;// date de passage en Nb_stations + 1
	REC_[Nb_stations + 1][0].perf = 0; //coût en Nb_stations + 1
	REC_[Nb_stations + 1][0].succ = -1; //liste chainee (non utilisee)
	REC_[Nb_stations + 1][0].jant = -1; //indice du pere dans REC[i+1]
	REC_[Nb_stations + 1][0].xx = -1; //decision : xx = 0 => on va a la station suivante, xx = 1 => on va au depot
	REC_[Nb_stations + 1][0].tr = 0; //actif si 0, inactif si -1



	//int j0;
	for (int i = Nb_stations; i >= 0; i--)
	{
		int K = 0;
		REC[i] = -1;

		int suiv_list = 0;

		//on parcourt REC_[i+1] et on cree les etats a mettre dans REC[i]
		while (REC_[i + 1][suiv_list].tr == 0)
		{
			//si energie en (i+1) + energie (i->i+1) + energie (usine -> i) <= capa Reservoir 
			//on est arrive en i+1 en venant de i
			if ((REC_[i + 1][suiv_list].V + E_i[i] <= vmax) && (REC_[i + 1][suiv_list].T + d_i[i] <= TMAX))//+ E_0i[i] 
			{
				ETAT[1].V = REC_[i + 1][suiv_list].V + E_i[i];
				ETAT[1].perf = REC_[i + 1][suiv_list].perf + (alpha * E_i[i]) + (beta*d_i[i]);
				ETAT[1].T = REC_[i + 1][suiv_list].T + d_i[i];
				ETAT[1].xx = 0;
				ETAT[1].succ = REC_[i + 1][suiv_list].succ;
				ETAT[1].jant = suiv_list;

			}
			else
			{
				ETAT[1].V = INFINI;
			}
			// si energie en(i + 1) + energie(usine->i+1) <= capa Reservoir
			//on est arrive en i+1 en venant de l'usine
			if ((REC_[i + 1][suiv_list].V + E_0i[i + 1] <= vmax) && (REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p <= TMAX))
			{
				ETAT[2].V = E_i0[i];
				ETAT[2].perf = REC_[i + 1][suiv_list].perf + (alpha * (E_i0[i] + E_0i[i + 1])) + (beta*(d_i0[i] + d_0i[i + 1] + p));
				ETAT[2].T = REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p;
				ETAT[2].xx = 1;
				ETAT[2].succ = REC_[i + 1][suiv_list].succ;
				ETAT[2].jant = suiv_list;
			}
			else
			{
				ETAT[2].V = INFINI;

			}
			for (int s = 1; s <= 2; s++)
			{
				if ((ETAT[s].V != INFINI) && (ETAT[s].T + TINF[i] <= TMAX))
				{
					if (REC[i] == -1)
					{
						list_recharge.push_back(ETAT[s]);
						REC[i] = 0;
					}
					else
					{
						bool boo = true;
						int ind_perf = 2;
						it = list_recharge.begin();
						//parcourt la liste des etats, si on a un etat identique (meme V) deja present et meilleur on ecarte s (ind_perf  = 0)
						//si on a un etat identique moins bon => on le remplace (ind_perf = 1)

						while (it != list_recharge.end() && boo == true)
						{

							//si ETAT[s] est meilleur que it on remplace it par ETAT[s] :
							if (it->V >= ETAT[s].V)
							{
								if (ETAT[s].perf < it->perf)
								{
									ind_perf = 1;
									it->V = ETAT[s].V; //!!!!!!!!!!!!!!!!!!!!!ajouter le 10/ 06/ 2020
									it->perf = ETAT[s].perf;
									it->jant = ETAT[s].jant;
									it->xx = ETAT[s].xx;
									it->T = ETAT[s].T;
									boo = false;
								}
							}
							//si it est meilleur que ETAT[s] => ETAT[s] n'est pas ajoute
							if (it->V <= ETAT[s].V && it->perf <= ETAT[s].perf)
								ind_perf = 0;

							++it;
						}

						//on n a pas trouve d etat identique => on insere
						if (ind_perf == 2)
							list_recharge.push_back(ETAT[s]);
					}


				}
			}

			suiv_list = suiv_list + 1;
		}


		K = 0;
		for (it = list_recharge.begin(); it != list_recharge.end(); ++it)
		{
			//cout << "i=" << K << " : " << "(";
			REC_[i][K].V = it->V;
			REC_[i][K].perf = it->perf;
			Energie[i][REC_[i][K].V] = REC_[i][K].perf; //quantité d'energie minimal qu'il faut pour aller de i au dépôt avec un reservoir REC_[i][ind_tour].V

			REC_[i][K].jant = it->jant;
			REC_[i][K].xx = it->xx;
			REC_[i][K].T = it->T;
			//REC_[i][K].succ = K - 1;
			REC_[i][K].tr = 0;
			K = K + 1;
		}

		//cout << "Nombre d'etats " << list_recharge.size() << endl;
		//cout << endl;
		list_recharge.clear();
	}//fin pour principal


	///reinitialisation de REC et REC_

	for (int i = 0; i < Nb_stations + 2; i++)
	{
		REC[i] = -1;
		for (int j = 0; j < NB_ETAT_RECHARG + 1; j++)
		{
			REC_[i][j].succ = -1;
			REC_[i][j].jant = -1; //indice la station suivante générée dans REC[i+1]
			REC_[i][j].xx = -1; // indice de la décision à prendre pour quitter de i à i+1
			REC_[i][j].tr = -1;

		}

	}

	//remplissage du tableau energie pour les autres valeurs de l'énergie min de i à 0



	for (int j = 0; j <= Nb_stations + 1; j++)
	{
		int enrg = -1;
		for (int h = 0; h <= vmax; ++h)
		{
			//si Temps j, h non calcule alors on cherche le prochain k > h tel que Temps[j][k] est connu
			if (Energie[j][h] == -1)
			{
				Energie[j][h] = enrg;
			}
			else
				enrg = Energie[j][h];
#ifdef VERIF_
			//le temps pour aller de j vers fin sans compter les recharges doit etre <= au temps en comptant les recharges
			if (Energie[j][h] != -1 && E__[j] > Energie[j][h])
				stopProg("Heuristique::dyn_Recharge_energie");
#endif
		}

	}


}

//cet algorithme permet d'avoir les bornes inférieurs pour le temps (utile pour le filtrage du programme globale)
void Heuristique::dyn_Recharge_temps() // [p OK]
{
	int alpha = 0;
	int beta = 1;

	std::list <Etat_Recharg> list_recharge;
	std::list<Etat_Recharg>::iterator it;

	REC[Nb_stations + 1] = 0;
	REC_[Nb_stations + 1][0].V = v0;//énergie initial dans le véhicule 
	REC_[Nb_stations + 1][0].T = 0;// date de passage 
	REC_[Nb_stations + 1][0].perf = 0; //coût 
	REC_[Nb_stations + 1][0].succ = -1; //liste chainee (non utilisee)
	REC_[Nb_stations + 1][0].jant = -1; //indice du pere dans REC[i+1]
	REC_[Nb_stations + 1][0].xx = -1; //decision : xx = 0 => on va a la station suivante, xx = 1 => on va au depot
	REC_[Nb_stations + 1][0].tr = 0; //actif si 0, inactif si -1



	//int j0;
	for (int i = Nb_stations; i >= 0; i--)
	{
		int K = 0;
		REC[i] = -1;

		int suiv_list = 0;

		//on parcourt REC_[i+1] et on cree les etats a mettre dans REC[i]
		while (REC_[i + 1][suiv_list].tr == 0)
		{
			//si energie en (i+1) + energie (i->i+1) + energie (usine -> i) <= capa Reservoir 
			//on est arrive en i+1 en venant de i
			if ((REC_[i + 1][suiv_list].V + E_i[i] <= vmax) && (REC_[i + 1][suiv_list].T + d_i[i] <= TMAX))// + E_0i[i]
			{
				ETAT[1].V = REC_[i + 1][suiv_list].V + E_i[i];
				ETAT[1].perf = REC_[i + 1][suiv_list].perf + (alpha * E_i[i]) + (beta*d_i[i]);
				ETAT[1].T = REC_[i + 1][suiv_list].T + d_i[i];
				ETAT[1].xx = 0;
				ETAT[1].succ = REC_[i + 1][suiv_list].succ;
				ETAT[1].jant = suiv_list;

			}
			else
			{
				ETAT[1].V = INFINI;
			}
			// si energie en(i + 1) + energie(usine->i+1) <= capa Reservoir
			//on est arrive en i+1 en venant de l'usine
			if ((REC_[i + 1][suiv_list].V + E_0i[i + 1] <= vmax) && (REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p <= TMAX))
			{
				ETAT[2].V = E_i0[i];
				ETAT[2].perf = REC_[i + 1][suiv_list].perf + alpha * (E_i0[i] + E_0i[i + 1]) + beta * (d_i0[i] + d_0i[i + 1] + p); //ajout duree periode = temps de recharge
				ETAT[2].T = REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p; //ajout duree periode = temps de recharge
				ETAT[2].xx = 1;
				ETAT[2].succ = REC_[i + 1][suiv_list].succ;
				ETAT[2].jant = suiv_list;
			}
			else
			{
				ETAT[2].V = INFINI;

			}
			for (int s = 1; s <= 2; s++)
			{
				if ((ETAT[s].V != INFINI) && (ETAT[s].T + TINF[i] <= TMAX))
				{
					if (REC[i] == -1)
					{
						list_recharge.push_back(ETAT[s]);
						REC[i] = 0;
					}
					else
					{

						int ind_perf = 2;
						it = list_recharge.begin();
						//parcourt la liste des etats, si on a un etat identique (meme V) deja present et meilleur on ecarte s (ind_perf  = 0)
						//si on a un etat identique moins bon => on le remplace (ind_perf = 1)

						while (it != list_recharge.end())
						{

							//si ETAT[s] est meilleur que it on remplace it par ETAT[s] :
							if (it->V >= ETAT[s].V)
							{
								if (ETAT[s].perf < it->perf)
								{
									ind_perf = 1;
									it->V = ETAT[s].V; //!!!!!!!!!!!!!!!!!!!!!ajouter le 10/ 06/ 2020
									it->perf = ETAT[s].perf;
									it->jant = ETAT[s].jant;
									it->xx = ETAT[s].xx;
									it->T = ETAT[s].T;
								}
							}
							//si it est meilleur que ETAT[s] => ETAT[s] n'est pas ajoute
							if (it->V <= ETAT[s].V && it->perf <= ETAT[s].perf)
								ind_perf = 0;

							++it;
						}

						//on n a pas trouve d etat identique => on insere
						if (ind_perf == 2)
							list_recharge.push_back(ETAT[s]);
					}


				}
			}

			suiv_list = suiv_list + 1;
		}


		K = 0;
		for (it = list_recharge.begin(); it != list_recharge.end(); ++it)
		{
			//cout << "i=" << K << " : " << "(";
			REC_[i][K].V = it->V;
			REC_[i][K].perf = it->perf;
			Temps[i][REC_[i][K].V] = REC_[i][K].perf; //quantité de temps minimal qu'il faut pour aller de i au dépôt avec un reservoir REC_[i][ind_tour].V

			REC_[i][K].jant = it->jant;
			REC_[i][K].xx = it->xx;
			REC_[i][K].T = it->T;
			//REC_[i][K].succ = K - 1;
			REC_[i][K].tr = 0;
			K = K + 1;
		}

		//cout << "Nombre d'etats " << list_recharge.size() << endl;
		//cout << endl;
		list_recharge.clear();
	}//fin pour principal


	//reinitialisation de REC et REC_
	for (int i = 0; i < Nb_stations + 2; i++)
	{
		REC[i] = -1;
		for (int j = 0; j < NB_ETAT_RECHARG + 1; j++)
		{
			REC_[i][j].succ = -1;
			REC_[i][j].jant = -1; //indice la station suivante générée dans REC[i+1]
			REC_[i][j].xx = -1; // indice de la décision à prendre pour quitter de i à i+1
			REC_[i][j].tr = -1;

		}

	}

	//remplissage du tableau temps pour les autres valeurs( temps min de i à 0)
	for (int j = 0; j <= Nb_stations + 1; j++)
	{
		int tmps = -1;
		for (int h = 0; h <= vmax; ++h)
		{
			//si Temps j, h non calcule alors on cherche le prochain k > h tel que Temps[j][k] est connu
			if (Temps[j][h] == -1)
			{
				Temps[j][h] = tmps;
			}
			else
				tmps = Temps[j][h];
#ifdef VERIF_
			//le temps pour aller de j vers fin sans compter les recharges doit etre <= au temps en comptant les recharges
			if (Temps[j][h] != -1 && d__[j] > Temps[j][h])
				stopProg("Heuristique::dyn_Recharge_temps : probleme");
#endif
		}

	}


#ifdef VERIF_
	for (int j = 0; j <= Nb_stations + 1; j++)
	{

		int h = 2;

		while (h < vmax && (Temps[j][h - 1] == -1 || Temps[j][h] <= Temps[j][h - 1]))
			h++;

		if (h < vmax)
			stopProg("dyn_Recharge_temps : on met plus de temps avec plus d'H2");

	}
#endif

}


bool  Creer_instance::Creation_instances(int p, int xrectangle, int yrectangle, int xrendement,
	int xcout, int NB_stations, int alpha, int beta,
	int cout_fixe, int Num_instance, int citerne_initiale,
	int param_cit, int param_v0, int param_vmax, int param_tmax) //!!!!!!!!!!!!!!!!!!!!important : ce code doit encore être corrigé pour tenir compte de p>1
{
	//p=taille d'une période
	//(xrectangle,yrectangle)=longueur en abscisse et en ordonné du rectangle dans lequel les stations seront placées
	//xcout=  le cout est compris entre 1 et xcout
	//xrendement= le nombre de production à faire pour remplir la citerne
	//NB_stations = nombre de stations de l'instance 
	//Tmax = temps maximal
	//alpha = paramètre de l'énergie
	// beta = paramètre du temps
	//cout_fixe = cout de setup
	// Num_instance = numéro de l'instance
	//citerne_initiale =  quantité de H2 initiale dans la citerne
	//cit =  (param_cit*vmax) //param_cit = paramètre qui permet de determiner la capacité maximale de la citerne cit en fonction de le capacité du véhicule vmax
	//v0 = d_i0[0] + param_v0 // param_v0 = paramètre qui permet de determiner l'énergie initiale dans le véhicule
	//v0 >= d_i0[0]
	//vmax = (E__[0] / param_vmax) + v0 // param_vmax = paramètre qui permet de determiner la capacité maximale du véhicule
	//Tmax >= param_tmax*d__[0];

	//Description fichier instance :
	// NB_stations ----> nombre de stations
	//Tmax  ----> Temps maximal
	// v0  ----> Hydrogène initiale du véhicule
	//vmax  ----> capacité du reservoir d'hydrogène du véhicule
	// p   ----> Taille d'une période
	// alpha  ----> paramètre de l'énergie
	// beta  ----> paramètre du temps
	// cout_fixe  ----> cout fixe de production
	// citerne_initiale  ----> Hydrogène initiale dans la citerne 
	// cit  ---->  Capacité de la citerne
	//coordonées des stations
	bool bool_etat_creer = false;
	vector<pair<int, int>> verif_vect;
	srand(static_cast<unsigned int>(time(NULL)));
	int cit, vmax, v0;
	float distance = 0, energie = 0;
	string j_a, NB_stations_a, Tmax_a;
	int a, b;
	vector<vector<int>>list_stations;
	vector<int> rendement;
	vector<int> cout_;
	pair<int, int> depot(-1, -1); //Pour éviter que l'usine et le dépôt n'ait la même position on va conserver les coordonnées du dépôt dans depot
	pair<int, int> usine(-1, -1);
	int usiney = -1, usinex = -1;
	list_stations.resize(2, vector<int>(NB_stations + 1));

	verif_vect.clear();

	for (int i = 0; i <= NB_stations; i++)
	{
		a = rand() % xrectangle;
		b = rand() % yrectangle;
		pair<int, int> stat_doubl(a, b);
		while (find(verif_vect.begin(), verif_vect.end(), stat_doubl) != verif_vect.end()
			|| (stat_doubl.first == usinex && stat_doubl.second == usiney))
			//|| find(verif_vect.begin(), verif_vect.end(), usine) != verif_vect.end())
		{
			a = rand() % xrectangle;
			b = rand() % yrectangle;
			stat_doubl = { a, b };

		}
		list_stations[0][i] = a;
		list_stations[1][i] = b;
		pair<int, int> depot(list_stations[0][0], list_stations[1][0]);
		if (i == 0)
		{
			pair<int, int> usine(list_stations[0][0], list_stations[1][0] + 1);
			usinex = list_stations[0][0];
			usiney = list_stations[1][0] + 1;
		}
		verif_vect.push_back({ a, b });


	}
	for (int i = 1; i <= NB_stations; i++)
	{
		float tempp1 = sqrt(pow(abs(list_stations[0][i] - list_stations[0][i - 1]), 2) + pow(abs(list_stations[1][i] - list_stations[1][i - 1]), 2));
		distance = distance + static_cast<int>(ceil(tempp1));
		energie = energie + abs(list_stations[0][i] - list_stations[0][i - 1]) + abs(list_stations[1][i] - list_stations[1][i - 1]);
	}
	float tempp2 = sqrt(pow(abs(list_stations[0][NB_stations] - list_stations[0][0]), 2) + pow(abs(list_stations[1][NB_stations] - list_stations[1][0]), 2));
	distance = distance + static_cast<int>(ceil(tempp2));
	energie = energie + abs(list_stations[0][NB_stations] - list_stations[0][0]) + abs(list_stations[1][NB_stations] - list_stations[1][0]);
	v0 = 1 + param_v0;// car d_i0[0]=1 //2 * (abs(list_stations[0][1] - list_stations[0][0]) + abs(list_stations[1][1] - list_stations[1][0]))
	float tempp3 = energie / param_vmax;
	vmax = static_cast<int>(ceil(tempp3)) + v0;
	cit = static_cast<int> (param_cit*vmax);
	while (cit < citerne_initiale) cit++;
	int Tmax = param_tmax * distance;
	while (Tmax%p != 0) Tmax++;
	rendement.resize((Tmax / p) + 1);
	cout_.resize((Tmax / p) + 1);

	for (int t = 0; t < (Tmax / p) + 1; t++)
	{

		rendement[t] = rand() % (cit / xrendement) + 1;//
		cout_[t] = rand() % xcout + 1;


	}
	j_a = to_string(Num_instance);
	NB_stations_a = to_string(NB_stations);
	Tmax_a = to_string(Tmax);
	string const nomFichier("instances_avec_p/instance__" + j_a + ".txt");

	cout << "test" << endl;
	if (vmax*cit*NB_stations*ceil(Tmax / p) * 2 <= 1000000)//seuil du nombre d'état max  à créer
	{
		cout << "creer" << endl;
		ofstream monFlux(nomFichier.c_str());
		if (monFlux)   //On teste si tout est OK
		{

			monFlux << NB_stations << endl;//nombre de stations
			monFlux << Tmax << endl;//TMAX
			monFlux << v0 << endl;
			monFlux << vmax << endl;
			monFlux << p << endl;//delta devient p la taille d'une période
			monFlux << alpha << endl;//alpha
			monFlux << beta << endl;//beta
			monFlux << cout_fixe << endl;//cout fixe
			monFlux << citerne_initiale << endl;
			monFlux << cit << endl;
			monFlux << " " << endl;
			monFlux << "stations" << endl;
			for (int i = 0; i <= NB_stations; i++)
			{
				monFlux << list_stations[0][i] << endl;
				monFlux << list_stations[1][i] << endl;
			}
			bool_etat_creer = true;

		}
		else
		{
			cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
		}

		monFlux << "FIN" << endl;

		string const nomFichier_prod("instances_avec_p/instance_Prod__" + j_a + ".txt");
		ofstream monFlux_prod(nomFichier_prod.c_str());
		if (monFlux_prod)  //On teste si tout est OK
		{
			for (int t = 0; t < (Tmax / p) + 1; t++)
			{
				monFlux_prod << rendement[t] << endl;

			}
			for (int t = 0; t < (Tmax / p) + 1; t++)
			{

				monFlux_prod << cout_[t] << endl;
			}
		}
		else
		{
			cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
		}
	}//seuil du nombre d'état max  à créer
	return bool_etat_creer;
}


//
bool  Creer_instance::Creation_instances_20periodes(int xrectangle, int yrectangle, int xrendement,
	int xcout, int NB_stations, int alpha, int beta,
	int cout_fixe, int Num_instance, int citerne_initiale,
	int param_cit, int param_v0, int param_vmax, int param_tmax) //!!!!!!!!!!!!!!!!!!!!important : ce code doit encore être corrigé pour tenir compte de p>1
{

	//on veut 20 periodes exactement, changer ce nombre pour un autre nombre de periodes
	int nbPeriode = 20;


	//p=taille d'une période
	//(xrectangle,yrectangle)=longueur en abscisse et en ordonné du rectangle dans lequel les stations seront placées
	//xcout=  le cout est compris entre 1 et xcout
	//xrendement= le nombre de production à faire pour remplir la citerne
	//NB_stations = nombre de stations de l'instance 
	//Tmax = temps maximal
	//alpha = paramètre de l'énergie
	// beta = paramètre du temps
	//cout_fixe = cout de setup
	// Num_instance = numéro de l'instance
	//citerne_initiale =  quantité de H2 initiale dans la citerne
	//cit =  (param_cit*vmax) //param_cit = paramètre qui permet de determiner la capacité maximale de la citerne cit en fonction de le capacité du véhicule vmax
	//v0 = d_i0[0] + param_v0 // param_v0 = paramètre qui permet de determiner l'énergie initiale dans le véhicule
	//v0 >= d_i0[0]
	//vmax = (E__[0] / param_vmax) + v0 // param_vmax = paramètre qui permet de determiner la capacité maximale du véhicule
	//Tmax >= param_tmax*d__[0];

	//Description fichier instance :
	// NB_stations ----> nombre de stations
	//Tmax  ----> Temps maximal
	// v0  ----> Hydrogène initiale du véhicule
	//vmax  ----> capacité du reservoir d'hydrogène du véhicule
	// p   ----> Taille d'une période
	// alpha  ----> paramètre de l'énergie
	// beta  ----> paramètre du temps
	// cout_fixe  ----> cout fixe de production
	// citerne_initiale  ----> Hydrogène initiale dans la citerne 
	// cit  ---->  Capacité de la citerne
	//coordonées des stations
	bool bool_etat_creer = false;
	vector<pair<int, int>> verif_vect;
	//srand(static_cast<unsigned int>(time(NULL)));
	int cit, vmax, v0;
	float distance = 0, energie = 0;
	string j_a, NB_stations_a, Tmax_a;
	int a, b;
	vector<vector<int>>list_stations;
	vector<int> rendement;
	vector<int> cout_;
	pair<int, int> depot(-1, -1); //Pour éviter que l'usine et le dépôt n'ait la même position on va conserver les coordonnées du dépôt dans depot
	pair<int, int> usine(-1, -1);
	int usiney = -1, usinex = -1;
	list_stations.resize(2, vector<int>(NB_stations + 1));

	verif_vect.clear();

	for (int i = 0; i <= NB_stations; i++)
	{
		a = rand() % xrectangle;
		b = rand() % yrectangle;
		pair<int, int> stat_doubl(a, b);
		while (find(verif_vect.begin(), verif_vect.end(), stat_doubl) != verif_vect.end()
			|| (stat_doubl.first == usinex && stat_doubl.second == usiney))
			//|| find(verif_vect.begin(), verif_vect.end(), usine) != verif_vect.end())
		{
			a = rand() % xrectangle;
			b = rand() % yrectangle;
			stat_doubl = { a, b };

		}
		list_stations[0][i] = a;
		list_stations[1][i] = b;
		pair<int, int> depot(list_stations[0][0], list_stations[1][0]);
		if (i == 0)
		{
			pair<int, int> usine(list_stations[0][0], list_stations[1][0] + 1);
			usinex = list_stations[0][0];
			usiney = list_stations[1][0] + 1;
		}
		verif_vect.push_back({ a, b });


	}
	for (int i = 1; i <= NB_stations; i++)
	{
		float tempp1 = sqrt(pow(abs(list_stations[0][i] - list_stations[0][i - 1]), 2) + pow(abs(list_stations[1][i] - list_stations[1][i - 1]), 2));
		distance = distance + static_cast<int>(ceil(tempp1));
		energie = energie + abs(list_stations[0][i] - list_stations[0][i - 1]) + abs(list_stations[1][i] - list_stations[1][i - 1]);
	}
	float tempp2 = sqrt(pow(abs(list_stations[0][NB_stations] - list_stations[0][0]), 2) + pow(abs(list_stations[1][NB_stations] - list_stations[1][0]), 2));
	distance = distance + static_cast<int>(ceil(tempp2));
	energie = energie + abs(list_stations[0][NB_stations] - list_stations[0][0]) + abs(list_stations[1][NB_stations] - list_stations[1][0]);
	v0 = 1 + param_v0;// car d_i0[0]=1 //2 * (abs(list_stations[0][1] - list_stations[0][0]) + abs(list_stations[1][1] - list_stations[1][0]))
	float tempp3 = energie / param_vmax;
	vmax = static_cast<int>(ceil(tempp3)) + v0;
	cit = static_cast<int> (param_cit*vmax);
	while (cit < citerne_initiale) cit++;
	int Tmax = param_tmax * distance;


	while (Tmax%nbPeriode != 0) Tmax++;

	//on calcule p pour que nbPeriodes * p = TMax
	int p = Tmax / nbPeriode;

	rendement.resize((Tmax / p) + 1);
	cout_.resize((Tmax / p) + 1);

	for (int t = 0; t < (Tmax / p) + 1; t++)
	{

		rendement[t] = rand() % (cit / xrendement) + 1;//
		cout_[t] = rand() % xcout + 1;


	}
	j_a = to_string(Num_instance);
	NB_stations_a = to_string(NB_stations);
	Tmax_a = to_string(Tmax);
	string const nomFichier("instances_avec_p/instance__" + j_a + ".txt");

	cout << "test" << endl;
	if (vmax*cit*NB_stations*ceil(Tmax / p) * 2 <= 1000000)//seuil du nombre d'état max  à créer
	{
		cout << "creer" << endl;
		ofstream monFlux(nomFichier.c_str());
		if (monFlux)   //On teste si tout est OK
		{

			monFlux << NB_stations << endl;//nombre de stations
			monFlux << Tmax << endl;//TMAX
			monFlux << v0 << endl;
			monFlux << vmax << endl;
			monFlux << p << endl;//delta devient p la taille d'une période
			monFlux << alpha << endl;//alpha
			monFlux << beta << endl;//beta
			monFlux << cout_fixe << endl;//cout fixe
			monFlux << citerne_initiale << endl;
			monFlux << cit << endl;
			monFlux << " " << endl;
			monFlux << "stations" << endl;
			for (int i = 0; i <= NB_stations; i++)
			{
				monFlux << list_stations[0][i] << endl;
				monFlux << list_stations[1][i] << endl;
			}
			bool_etat_creer = true;

		}
		else
		{
			cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
		}

		monFlux << "FIN" << endl;

		string const nomFichier_prod("instances_avec_p/instance_Prod__" + j_a + ".txt");
		ofstream monFlux_prod(nomFichier_prod.c_str());
		if (monFlux_prod)  //On teste si tout est OK
		{
			for (int t = 0; t < (Tmax / p) + 1; t++)
			{
				monFlux_prod << rendement[t] << endl;

			}
			for (int t = 0; t < (Tmax / p) + 1; t++)
			{

				monFlux_prod << cout_[t] << endl;
			}
		}
		else
		{
			cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
		}
	}//seuil du nombre d'état max  à créer
	return bool_etat_creer;
}
/******************************Estimation production****************************************/
void Heuristique::Cout_min_prod() //[p OK]
{

	int P1;
	//if(PMAX==0) {
	VAL[Nombre_de_periode][0][0] = 0;
	VAL[Nombre_de_periode][1][0] = 0;
	//}else 
	//{
	for (int P = 1; P < PMAX; P++)
	{
		VAL[Nombre_de_periode][0][P] = INFINI; //10000 vaut l'infini
		VAL[Nombre_de_periode][1][P] = INFINI;
	}

	//}
	//cout << "t=" << TMAX-1<< "P=" << PMAX << "E=" << 0 << "  " << VAL[TMAX-1][0][PMAX]<< endl;

	for (int t = Nombre_de_periode - 1; t >= 0; t--)
	{

		for (int P = 1; P <= PMAX; P++)
		{
			for (int E = 0; E < 2; E++)
			{
				VAL[t][E][0] = 0;
				if (P > q__[t])
				{
					VAL[t][E][P] = INFINI;/*impossible de produire Pà partir de t+1*/

				}
				else
				{

					{
						P1 = max(P - rendement[t], 0);/* rendement[t ] production en t,t+1*/
						int C1 = coutv[t] + (1 - E)*Cf; //remplacer coutv[t ]  
						//on ne produit pas
						int P2 = P;
						int C2 = 0;
						if ((C1 + VAL[t + 1][1][P1]) > (C2 + VAL[t + 1][0][P2]))
						{
							VAL[t][E][P] = C2 + VAL[t + 1][0][P2]; /*cout min de production de la quantité P si on produit à partir de la date t*/
						}
						else
						{
							VAL[t][E][P] = C1 + VAL[t + 1][1][P1];
							//cout << "t=" << t << "P=" << P << "E=" << E << "  " << VAL[t][E][P] << endl;
						}
					}

				}
				//cout << "t=" << t << "P=" << P << "E=" << E << "  " << VAL[t][E][P] << endl;

			}
		}
	}

}

/***************************************Data***********************************************/
void Heuristique::read_instance_in_file(string path, string num_file, string path2)
{
	cout << "Fonction read instance" << endl;
	//lecture de la solution BSUP pour le programme dynamique global
	string ligne;
#ifdef BSUP_Greedy_10
	int ibsup;
	std::string cheminbsup = "cout_BSUP_Greedy_10.txt";
#endif
#ifdef BSUP_Greedy_20
	float BSUP_20[COUT_BSUP_INST];
	std::string cheminbsup1 = "cout_BSUP_Greedy_20.txt";
	ifstream monreadFlux_instancesbsup1(cheminbsup1);
	if (!monreadFlux_instancesbsup1)
	{
		stopProg("Heuristique::read_instance_in_file:fichier non ouvert");
	}

	getline(monreadFlux_instancesbsup1, ligne);
	ibsup = 0;
	while (!monreadFlux_instancesbsup1.eof()) //Tant qu'on n'est pas à la fin, on lit
	{
#ifdef VERIF_
		if (ibsup >= COUT_BSUP_INST)
			stopProg("Heuristique::read_instance_in_file : on depasse COUT_BSUP_INST");
#endif
		BSUP_20[ibsup] = static_cast<float>(stoi(ligne));
		getline(monreadFlux_instancesbsup1, ligne);
		ibsup = ibsup + 1;
	}
	//BSUP[ibsup] = stoi(ligne);

	monreadFlux_instancesbsup1.close();

#endif
#ifdef BSUP_Greedy_50
	float BSUP_50[COUT_BSUP_INST];
	std::string cheminbsup2 = "cout_BSUP_Greedy_50.txt";
	ifstream monreadFlux_instancesbsup2(cheminbsup2);
	if (!monreadFlux_instancesbsup2)
	{
		stopProg("Heuristique::read_instance_in_file:fichier non ouvert");
	}

	getline(monreadFlux_instancesbsup2, ligne);
	ibsup = 0;
	while (!monreadFlux_instancesbsup2.eof()) //Tant qu'on n'est pas à la fin, on lit
	{
#ifdef VERIF_
		if (ibsup >= COUT_BSUP_INST)
			stopProg("Heuristique::read_instance_in_file : on depasse COUT_BSUP_INST");
#endif
		BSUP_50[ibsup] = static_cast<float>(stoi(ligne));
		getline(monreadFlux_instancesbsup2, ligne);
		ibsup = ibsup + 1;
	}
	//BSUP[ibsup] = stoi(ligne);

	monreadFlux_instancesbsup2.close();

#endif

#ifdef BSUP_Pipeline
	std::string cheminbsup = "cout_BSUP_Pipeline.txt";
#endif
#ifdef BSUP_PL_1_seconde
	std::string cheminbsup = "cout_BSUP_PL_1_sec.txt";
#endif
#ifdef BSUP_Greedy_1
	std::string cheminbsup = "cout_BSUP.txt";
#endif
#ifdef BSUP_Helene
	std::string cheminbsup = "cout_BSUP_Helene.txt";
	ifstream monreadFlux_instancesbsup(cheminbsup);
	if (!monreadFlux_instancesbsup)
	{
		stopProg("Heuristique::read_instance_in_file:fichier non ouvert");
	}

	getline(monreadFlux_instancesbsup, ligne);
	ibsup = 0;
	while (!monreadFlux_instancesbsup.eof()) //Tant qu'on n'est pas à la fin, on lit
	{
#ifdef VERIF_
		if (ibsup >= COUT_BSUP_INST)
			stopProg("Heuristique::read_instance_in_file : on depasse COUT_BSUP_INST");
#endif
		BSUP[ibsup] = static_cast<float>(stoi(ligne));
		getline(monreadFlux_instancesbsup, ligne);
		ibsup = ibsup + 1;
	}
	//BSUP[ibsup] = stoi(ligne);

	monreadFlux_instancesbsup.close();

#endif
#ifdef BSUP_prod_Helene
	std::string cheminbsup = "cout_BSUP_Helene.txt";
#endif
#ifdef BSUP_Greedy_100 //toujours exécuter cette partie pour le prog dyn global car c'est ici qu'on compare les BSUP
	float BSUP_100[COUT_BSUP_INST];
	std::string cheminbsup3 = "cout_BSUP_Greedy_100.txt";
	ifstream monreadFlux_instancesbsup3(cheminbsup3);
	if (!monreadFlux_instancesbsup3)
	{
		stopProg("Heuristique::read_instance_in_file:fichier non ouvert");
	}

	getline(monreadFlux_instancesbsup3, ligne);
	ibsup = 0;
	while (!monreadFlux_instancesbsup3.eof()) //Tant qu'on n'est pas à la fin, on lit
	{
#ifdef VERIF_
		if (ibsup >= COUT_BSUP_INST)
			stopProg("Heuristique::read_instance_in_file : on depasse COUT_BSUP_INST");
#endif
		BSUP_100[ibsup] = static_cast<float>(stoi(ligne));
		//Comparaison des BSUP  pour garder la plus petite
		if (BSUP[ibsup] > 0)
		{
			if (BSUP_20[ibsup] > 0)BSUP[ibsup] = min(BSUP_20[ibsup], BSUP[ibsup]);
			if (BSUP_50[ibsup] > 0)BSUP[ibsup] = min(BSUP_50[ibsup], BSUP[ibsup]);
			if (BSUP_100[ibsup] > 0)BSUP[ibsup] = min(BSUP_100[ibsup], BSUP[ibsup]);
		}
		else
		{
			if (BSUP_20[ibsup] > 0)
			{
				if (BSUP_50[ibsup] > 0)BSUP[ibsup] = min(BSUP_50[ibsup], BSUP_20[ibsup]);
				if (BSUP_100[ibsup] > 0)BSUP[ibsup] = min(BSUP_100[ibsup], BSUP_20[ibsup]);
			}
			else
			{
				if (BSUP_50[ibsup] > 0)
				{
					if (BSUP_100[ibsup] > 0)BSUP[ibsup] = min(BSUP_100[ibsup], BSUP_50[ibsup]);
				}
				else
				{
					BSUP[ibsup] = BSUP_100[ibsup];
				}
			}
		}
		//Fin comparaison
		BSUP_prod_pipeline[ibsup] = BSUP[ibsup];
		getline(monreadFlux_instancesbsup3, ligne);
		ibsup = ibsup + 1;
	}
	//BSUP[ibsup] = stoi(ligne);

	monreadFlux_instancesbsup3.close();

#endif


	//lecture dela solution BSUP pour le pipeline //onnutilise plus cette BSUP car on a trop d'échec
#ifdef PIPELINE_dyn_Production_flexible__ancien
	cheminbsup = "cout_BSUP_prod_pipeline.txt";
	ifstream monreadFlux_instancesbsuppipe(cheminbsup);
	if (!monreadFlux_instancesbsuppipe)
	{
		stopProg("Heuristique::read_instance_in_file:fichier non ouvert");
	}

	getline(monreadFlux_instancesbsuppipe, ligne);
	ibsup = 0;
	while (!monreadFlux_instancesbsuppipe.eof()) //Tant qu'on n'est pas à la fin, on lit
	{
#ifdef VERIF_
		if (ibsup >= COUT_BSUP_INST)
			stopProg("Heuristique::read_instance_in_file : on depasse COUT_BSUP_INST");
#endif
		//selection de la meilleure bsup entre cout_BSUP_Helene et cout_BSUP_prod_pipeline pour le prod pipeline
		float bsup_pipe = static_cast<float>(stoi(ligne));
		if ((BSUP[ibsup] < bsup_pipe) && (BSUP[ibsup] > 0))
			BSUP_prod_pipeline[ibsup] = BSUP[ibsup];
		else
			BSUP_prod_pipeline[ibsup] = bsup_pipe;
		getline(monreadFlux_instancesbsuppipe, ligne);
		ibsup = ibsup + 1;
	}


	monreadFlux_instancesbsuppipe.close();
#endif // PIPELINE


	//lecture des stations
	std::string chemin;
	std::string chemin_{ ".txt" };
	chemin = path + num_file + chemin_;
	cout << "le chemin est :" << chemin << endl;
	ifstream monreadFlux_instances(chemin);
	if (!monreadFlux_instances)
		cout << "Heuristique::read : erreur : fichier non ouvert" << endl;

	if (monreadFlux_instances)  //On teste si tout est OK
	{
		//Tout est OK, on peut utiliser le fichier
		string ligne;


		monreadFlux_instances >> Nb_stations >> TMAX >> v0 >> vmax >> p >> alpha >> beta >> Cf >> s0 >> smax;//>> QMAX >> PMAX ...>> lamda >> coef_obj
		//alpha = beta = 1; //31/05/2022 : car dans les instances d'Aleandro ceci n'est pas toujours à 1
		QMAX = Nb_stations + 1; //Nombre de recharges maximales qu'on peut faire.
		int modulo = TMAX % p;
		if (modulo == 0)
		{
			Nombre_de_periode = TMAX / p;
		}
		else
		{
			Nombre_de_periode = TMAX / p + 1;
		}
		//Allocations de tableaux utilent (enlever de allocationTableaux()  car PMAX doit être avant d'exécuter allocationTableaux())
		list_stations.resize(2, vector<float>(Nb_stations + 3)); //ajout des coordonnees de l'usine a la fin (a l'indice NbStation+2)
		rendement.resize(Nombre_de_periode);
		coutv.resize(Nombre_de_periode);
		//Allocations de tableaux utilent
		while (getline(monreadFlux_instances, ligne) && ligne != "FIN") //Tant qu'on n'est pas à la fin, on lit
		{
			if (ligne.compare("stations") == 0) {
				getline(monreadFlux_instances, ligne);
				list_stations[0][Nb_stations + 1] = list_stations[0][0] = stoi(ligne);
				getline(monreadFlux_instances, ligne);
				list_stations[1][Nb_stations + 1] = list_stations[1][0] = stoi(ligne);
				int i = 1;

				getline(monreadFlux_instances, ligne);
				while (ligne != "FIN") //Tant qu'on n'est pas à la fin, on lit
				{

					list_stations[0][i] = stoi(ligne);

					getline(monreadFlux_instances, ligne);
					list_stations[1][i] = stoi(ligne);
					i = i + 1;

					getline(monreadFlux_instances, ligne);
				}
			}

		}


		// ajouter l'usine a la fin de list_stations = (x, y+1) avec (x,y) = depot
		//tournee dans list_station : 0, 1, ..., NbStation, depot Final
		//indice NbStation + 2 => coordonnes de l'usine (recharge)
		list_stations[0][Nb_stations + 2] = list_stations[0][0];
		list_stations[1][Nb_stations + 2] = list_stations[1][0] + 1;
	}
	else
	{
		cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
	}

	monreadFlux_instances.close();

	//==============================================================================
	//remplissage des tableaux "autres "a partir des tableaus lus

	std::string chemin2;
	chemin2 = path2 + num_file;
	data_prod(chemin2, num_file);
	allocationTableaux();
	data();

#ifdef	TRANSFORMER_DATA_P_DIFFERENT_DE_1
	data_prod_avec_p(path2, num_file);//Modifications des instances pour tenir compte de p!=1 //!!!!important
#endif


}



void Heuristique::allocationTableaux()
{


	//==============================================================================
	//ALLOCATIONS MEMOIRE

	//init des tableaux 1D avec TMAX

	q_.resize(Nombre_de_periode);
	q__.resize(Nombre_de_periode + 1);
	//init tableaux 1D avec nbStation
	d_i.resize(Nb_stations + 2); //d_i,i+1     /d
	d_0i.resize(Nb_stations + 2); //d_0,i      /d0*
	d_i0.resize(Nb_stations + 2); //d_i,0      /d0
	d_0.resize(Nb_stations + 2); //le détour delta + d(i0) + d(0i+1) - d(ii+1)  /d*
	d_.resize(Nb_stations + 2); // distance 0 à i
	d__.resize(Nb_stations + 2); // distance i à 0
	distmin.resize(Nb_stations + 2);
	E_i.resize(Nb_stations + 2); //E_i,i+1
	E_0i.resize(Nb_stations + 2); //E_0,i
	E_i0.resize(Nb_stations + 2);//E_i,0
	E_0.resize(Nb_stations + 2); //le détour  E(i0) + E(0i+1) - E(ii+1)
	E_.resize(Nb_stations + 2);// énergie 0 à i
	E__.resize(Nb_stations + 2); // énergie i à 0

	ORDOd.resize(Nb_stations + 1); //liste des stations ordonnées par détour (distance) croissant
	ORDOE.resize(Nb_stations + 1); //liste des stations ordonnées par détour (énergie) croissant

	EQ.resize(Nb_stations + 2); // énergie nécessaire pour aller du dépôt en i
	QQ.resize(Nb_stations + 2); //nombre de recharge à faire pour arriver en i
	TINF.resize(Nb_stations + 2); //temps minimal qu'il faut pour arriver en i

	x.resize(Nb_stations); //vecteur recharge x[i]=1 le véhicule ira se charger entre la station i et la station i+1
	REC.assign(Nb_stations + 2, -1);

	//tableau 1D avec QMAX
	u.resize(Nb_stations + 2); //vecteur recharge x[i]=1 le véhicule ira se charger entre la station i et la station i+1
	 //quantité d'hydrogène chargée à la recharge q
	u_.resize(Nb_stations + 2); //quantité d'hydrogène chargée aux recharges 1,2,... q
	A.resize(Nb_stations + 2); //borne inférieure pour la date de la recharge q
	B.resize(Nb_stations + 2); //borne supérieure pour la date de la recharge q

	// tableaux 2D


	Energie.resize(Nb_stations + 2, vector<int>(vmax + 1, -1)); //tableaux de l'energie min pour aller de i au dépot. ceci sera utilisé dans les filtrages)
	Temps.resize(Nb_stations + 2, vector<int>(vmax + 1, -1)); //tableaux du temps min pour aller de i au dépot. ceci sera utilisé dans les filtrages)


	//tableau 3D
	VAL.resize(Nombre_de_periode + 1, vector<vector<int>>(2, vector<int>(PMAX + 1)));
	REC_.resize(Nb_stations + 2, vector<Etat_Recharg>(NB_ETAT_RECHARG + 1));
	N_PROD.resize(Nombre_de_periode + 1);
	//PROD.resize(Nombre_de_periode+1, vector<Etat_prod>(NB_ETAT_prod));
	PROD.resize(Nombre_de_periode + 1);
	for (int i = 0; i < Nombre_de_periode + 1; ++i)
	{
		PROD[i].reserve(1000);
	}
	ETATT.resize(Nombre_de_periode + 1);
	IND.resize(Nb_stations + 2);

	Opt_energie.resize(Nombre_de_periode);
	Opt_temps.resize(Nombre_de_periode);

	//pipeline
	St.resize(Nb_stations + 2, -1);
	Deltaa.resize(Nb_stations + 2);
	BS.resize(Nb_stations + 2, -1);
	muS.resize(Nb_stations + 2, INFINI);
	mm.resize(Nb_stations + 2, -1);
	MM.resize(Nb_stations + 2, -1);
	DEC_Prod.resize(Nombre_de_periode + 1);
	SolutionPipelineProd.resize(Nombre_de_periode + 1);
	SolutionPipelineRech.resize(Nb_stations + 1);
	T_PIPELINE.resize(Nb_stations + 2);
}


int Heuristique::distanceEuclidienne(int i, int j)
{
	float tempp5 = (sqrt((list_stations[0][j] - list_stations[0][i])*(list_stations[0][j] - list_stations[0][i]) + (list_stations[1][j] - list_stations[1][i])*(list_stations[1][j] - list_stations[1][i])));
	return static_cast<int> (ceil(tempp5) + DBL_EPSILON);

}

int Heuristique::distanceManhattan(int i, int j)
{
	int d1 = static_cast<int>(abs(list_stations[0][j] - list_stations[0][i]));
	int d2 = static_cast<int> (abs(list_stations[1][j] - list_stations[1][i]));
	return d1 + d2; //Distance de Manhattan
}

void Heuristique::data()
{
	for (int t = 0; t < Nombre_de_periode; t++)
	{
		q__[t] = 0;
		for (int t1 = t; t1 < Nombre_de_periode; t1++)
		{

			q__[t] = q__[t] + rendement[t1]; //q__[t] est appelé Prod_max[i] dans l'article rairo et pipeline

		}

		q_[t] = 0;
		for (int t1 = 0; t1 < t; t1++)
		{
			q_[t] = q_[t] + rendement[t1];

		}
	}
	q__[Nombre_de_periode] = 0; // ????????? ajouté à cause du prod-max(i+1) de l'article pipeline
	for (int i = 0; i < Nb_stations + 1; i++)
	{
		d_i[i] = distanceEuclidienne(i, i + 1); // d[i][i + 1];
		d_0i[i] = distanceEuclidienne(Nb_stations + 2, i); //d[0][i];
		d_i0[i] = distanceEuclidienne(i, Nb_stations + 2); // d[i][0];


		E_i[i] = distanceManhattan(i, i + 1); //E[i][i + 1];
		E_0i[i] = distanceManhattan(Nb_stations + 2, i); // E[0][i];
		E_i0[i] = distanceManhattan(i, Nb_stations + 2);

	}


	d_0i[Nb_stations + 1] = d_0i[0];
	d_i0[Nb_stations + 1] = d_i0[0];
	E_0i[Nb_stations + 1] = E_0i[0];
	E_i0[Nb_stations + 1] = E_i0[0];

	for (int i = 0; i < Nb_stations + 1; i++)
	{
		d_0[i] = d_i0[i] + d_0i[i + 1] - d_i[i];
		E_0[i] = E_i0[i] + E_0i[i + 1] - E_i[i];
	}



	// ---- n existe pas : (a supprimer ??)
	d_i[Nb_stations + 1] = 0;
	E_i[Nb_stations + 1] = 0;
	d_0[Nb_stations + 1] = d_[0] = 0;
	E_0[Nb_stations + 1] = E_[0] = 0;
	//------

	for (int i = 0; i < Nb_stations + 2; i++)
	{
		//distance
		d__[i] = 0;
		for (int i1 = i; i1 < Nb_stations + 2; i1++)
		{

			d__[i] = d__[i] + d_i[i1];

		}

		//énergie
		E__[i] = 0;
		for (int i1 = i; i1 < Nb_stations + 2; i1++)
		{

			E__[i] = E__[i] + E_i[i1];

		}

	}


	for (int i = 1; i < Nb_stations + 2; i++)
	{
		d_[i] = 0;
		for (int i1 = 0; i1 <= i - 1; i1++)
		{
			d_[i] = d_[i] + d_i[i1];

		}

		E_[i] = 0;
		for (int i1 = 0; i1 <= i - 1; i1++)
		{
			E_[i] = E_[i] + E_i[i1];

		}
	}

	for (int i = 0; i < Nb_stations + 1; i++)
	{
		distmin[i] = 0;
		for (int j = i + 1; j < Nb_stations + 1; j++)
		{
			distmin[i] = distmin[i] + d_i[j];
		}
	}

}

void Heuristique::data_prod(string cheminprod_, string num_file)
{
	std::string chemi{ ".txt" };
	cheminprod_ = cheminprod_ + chemi;

#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
	string chemiReseauN = "InstanceReseauNeurones/RN_instance_prod__" + num_file;
	chemiReseauN = chemiReseauN + chemi;
	ofstream monwwFlux_instances_Reseau_Neuronne(chemiReseauN, ifstream::app);
#endif
	ifstream monwwFlux_instances(cheminprod_);
	float tempp6;
	int min=0, max=0;
	if (monwwFlux_instances.is_open())  //On teste si tout est OK
	{
		//Production
		PMAX = 0;
		for (int t = 0; t < Nombre_de_periode; t++)
		{
			monwwFlux_instances >> rendement[t];
			PMAX = PMAX + rendement[t];

#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
			monwwFlux_instances_Reseau_Neuronne << rendement[t] << " ";
#endif
		}
#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
		monwwFlux_instances_Reseau_Neuronne << endl;
		for (int t = 0; t < Nombre_de_periode; t++)
		{
			monwwFlux_instances_Reseau_Neuronne << Cf << " ";
		}
		monwwFlux_instances_Reseau_Neuronne << endl;
#endif
		moy_cout = 0;
		min = INFINI;
		max = 0;
		for (int t = 0; t < Nombre_de_periode; t++)
		{
			//Ct[t] = rand() % 10 + 1;
			monwwFlux_instances >> coutv[t];
			moy_cout += coutv[t];
			if (coutv[t] > max)max = coutv[t];
			if (coutv[t] < min)min = coutv[t];
#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
			monwwFlux_instances_Reseau_Neuronne << coutv[t] << " ";
#endif
		}

#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
		monwwFlux_instances_Reseau_Neuronne << endl;
		monwwFlux_instances_Reseau_Neuronne.close();
#endif
		tempp6 = moy_cout / (Nombre_de_periode);
		moy_cout = ceil(tempp6) + DBL_EPSILON;
	}
	ofstream ArmonFlux_instances_excel_EXP_m("moy_min_max_cout_var.csv", ifstream::app);

	ArmonFlux_instances_excel_EXP_m << num_file << ";" << tempp6 << ";" << min << ";" << max << endl;

	ArmonFlux_instances_excel_EXP_m.close();
	monwwFlux_instances.close();
}

void Heuristique::Estimation_energie_temps()
{

	vector<int> LE(Nb_stations + 1); //liste des stations ordonnees par détour énergetique croissant
	vector<int> LE1(Nb_stations + 1);
	vector<int> Ld(Nb_stations + 1); //liste des stations ordonnees par détour distance croissant
	vector<int> Ld1(Nb_stations + 1);

	//on initialise les listes de stations ordonees par distance et energie croissante
	for (int i = 0; i < Nb_stations + 1; i++)
	{
		LE[i] = ORDOE[i];
		Ld[i] = ORDOd[i];
		LE1[i] = -1;
		Ld1[i] = -1;
	}
#ifdef VERIF_
	for (int i = 0; i < Nb_stations; i++)
	{
		if (E_0[LE[i]] > E_0[LE[i + 1]])
			stopProg("Heuristique::Estimation_energie_temps : liste des stations ordonnees par detour energetique croissant mal ordonnee");

		if (d_0[Ld[i]] > d_0[Ld[i + 1]])
			stopProg("Heuristique::Estimation_energie_temps : liste des stations ordonnees par detour energetique croissant mal ordonnee");
	}
#endif

	EQ[0] = 0;// EQ[i]=Energie necessaire pour aller de 0 à i (avec les detours les plus courts possible)
	QQ[0] = 0; //QQ[0] = Nombre minimal de recharge à faire pour aller de 0 à i
	TINF[0] = 0; //TINF[i] = Temps minimale pour aller de 0 à i
	int EE, K, Q1;
	//On parcourt l'ensemble des stations 
	for (int i = 0; i <= Nb_stations; i++)
	{
		EE = EQ[i] + E_i[i]; //On calcule l'énergie qu'il faut pour aller de 0 à i+1
		if (i == Nb_stations) //de la derniere station vers le depot on veut en plus la quantité initiale v0
			EE += v0;

		K = QQ[i]; //On calcule le nombre de recharge qu'il faut pour aller de 0 à i
		float var1 = static_cast<float>(max(EE - v0, 0)); //On calcule l'énergie à produire pour aller de 0 à i+1
		float var = var1 / vmax; //On calcule le nombre minimum de recharge qu'il faut pour aller de 0 à i+1
		Q1 = static_cast<int> (ceil(var) + DBL_EPSILON); //ceil arrondi à l'entier supérieur
		while (Q1 != K)
		{
			int nbRecharge = 0;
			for (int j = 0; nbRecharge < (Q1 - K) && j <= Nb_stations; j++) //Q1 - K = nombre de recharge en plus à faire pour arriver en i+1
			{
				int station = LE[j];
				if (station <= i && (ORDOE[j] != -1))
				{
					LE1[j] = station;
					ORDOE[j] = -1;
					nbRecharge++;
				}
			}
			K = Q1; //K : nombre de recharge deja effectuees
			for (int l = 0; l < Nb_stations + 1; l++)
			{
				if (LE1[l] != -1)
				{
					EE = EE + E_0[LE1[l]];
					LE1[l] = -1;//on le desactive pour le princhain tour
				}
			}

			float var2 = 0.0;
			if (EE - v0 > 0) var2 = static_cast<float>(EE - v0);
			float var3 = var2 / vmax;
			//Q1 peut augmente car on vient de faire un détour
			Q1 = static_cast<int> (ceil(var3) + DBL_EPSILON); //ceil arrondi à l'entier supérieur
		}//fin while

		EQ[i + 1] = EE; //EQ[i] = energie min pour aller de depot initial a i (avec les detours les plus courts possible)
		QQ[i + 1] = K; //QQ[i] = nb de detours mininaux de depot initial a i

		int nbRecharge = 0;
		for (int j = 0; nbRecharge < (K - QQ[i]) && j <= Nb_stations; j++)
		{
			int station = Ld[j];
			if ((station <= i) && (ORDOd[j] != -1))
			{
				Ld1[j] = station;
				ORDOd[j] = -1;
				nbRecharge++;
			}
		}
		//cout << d_i[0]<<endl;
		TINF[i + 1] = TINF[i] + d_i[i];
		for (int l = 0; l < Nb_stations + 1; l++)
		{
			if (Ld1[l] != -1)
			{
				TINF[i + 1] = TINF[i + 1] + d_0[Ld1[l]];
				Ld1[l] = -1;
			}
		}
	}


}


//écrire une fonction qui ordonne les listes ORDOd et ORDOE par  détour croissant
//tri bulle des tableau ORDOd et ORDE
void Heuristique::Tri_bulle()
{
	int temp;

	for (int i = 0; i < Nb_stations + 1; i++)
	{
		ORDOE[i] = i;
		ORDOd[i] = i;
		//cout << "espace" << ORDOE[i] << endl;
	}

	for (int i = 0; i <= Nb_stations - 1; i++)
	{
		for (int k = Nb_stations; k > i; k--)
		{//- d[ORDOd[k]][ORDOd[k] + 1] et - d[ORDOd[k - 1]][ORDOd[k]]
			if (d_0[ORDOd[k]] < d_0[ORDOd[k - 1]])//d_i0[ORDOd[k]] + d_0i[ORDOd[k] + 1]  < d_i0[ORDOd[k - 1]] + d_0i[ORDOd[k - 1] + 1]

			{
				temp = ORDOd[k];
				ORDOd[k] = ORDOd[k - 1];
				ORDOd[k - 1] = temp;
			}
			//- E[ORDOE[k]][ORDOE[k] + 1]  et - E[ORDOE[k - 1]][ORDOE[k]]
			if (E_0[ORDOE[k]] < E_0[ORDOE[k - 1]])//E_i0[ORDOE[k]] + E_0i[ORDOE[k] + 1] < E_i0[ORDOE[k - 1]] + E_0i[ORDOE[k - 1] + 1]

			{
				temp = ORDOE[k];
				ORDOE[k] = ORDOE[k - 1];
				ORDOE[k - 1] = temp;
			}
		}
	}

#ifdef VERIF_

	int detour_d = 0;
	int detour_e = 0;
	for (int i = 0; i < Nb_stations + 1; i++)
	{
		int job1 = ORDOd[i];
		int job2 = ORDOE[i];

		if (d_0[job1] < detour_d)
			stopProg("Heuristique::Tri_bulle : tri distance faux");
		detour_d = d_0[job1];

		if (E_0[job2] < detour_e)
			stopProg("Heuristique::Tri_bulle : tri energie faux");
		detour_e = E_0[job2];
	}

#endif

}

void Heuristique::data_prod_avec_p(string chemin, string num_file)
{

	//string cheminn = chemin + "avecp_";
	//string chemin_co = cheminn + num_file;
	//string chemin_co_ext = chemin_co + ".txt";

	string const chemin_co_ext(chemin + "avecp_" + num_file + ".txt");
	ofstream fichier(chemin_co_ext.c_str());
	//ofstream fichier("chemin_co_ext");  // on ouvre le fichier en lecture

	if (fichier)  // si l'ouverture a réussi
	{
		// instructions
		int rend_p = 0, j = 0;
		for (int i = 0; i <= TMAX - 1; i++)
		{
			rend_p = rend_p + rendement[i];
			j++;
			if ((j == p) || (i == TMAX - 1))
			{
				fichier << rend_p << endl;
				rend_p = 0;
				j = 0;

			}
		}

		int cout_p = 0;
		j = 0;
		for (int i = 0; i <= TMAX - 1; i++)
		{
			cout_p = cout_p + coutv[i];
			j++;
			if ((j == p) || (i == TMAX - 1))
			{
				fichier << cout_p << endl;
				cout_p = 0;
				j = 0;
			}
		}

		fichier.close();  // on ferme le fichier
	}
	else  // sinon
	{
		cerr << "Impossible d'ouvrir le fichier !" << endl;
	}

}

void Heuristique::dyn_Recharge(float alpha, int beta)
{

	std::list <Etat_Recharg> list_recharge;
	std::list<Etat_Recharg>::iterator it;

	REC[Nb_stations + 1] = 0;
	REC_[Nb_stations + 1][0].V = v0;//énergie dans le véhicule en Nb_stations + 1
	REC_[Nb_stations + 1][0].T = 0;// date de passage en Nb_stations + 1
	REC_[Nb_stations + 1][0].perf = 0; //coût en Nb_stations + 1
	REC_[Nb_stations + 1][0].succ = -1; //liste chainee (non utilisee)
	REC_[Nb_stations + 1][0].jant = -1; //indice du pere dans REC[i+1]
	REC_[Nb_stations + 1][0].xx = -1; //decision : xx = 0 => on va a la station suivante, xx = 1 => on va au depot
	REC_[Nb_stations + 1][0].tr = 0; //actif si 0, inactif si -1

	//int j0;
	for (int i = Nb_stations; i >= 0; i--)
	{
		int K = 0;
		REC[i] = -1;

		int suiv_list = 0;

		//on parcourt REC_[i+1] et on cree les etats a mettre dans REC[i]
		while (REC_[i + 1][suiv_list].tr == 0)
		{
			//si energie en (i+1) + energie (i->i+1) + energie (usine -> i) <= capa Reservoir 
			//on est arrive en i+1 en venant de i
			if ((REC_[i + 1][suiv_list].V + E_i[i] <= vmax) && (REC_[i + 1][suiv_list].T + d_i[i] <= TMAX))//+ E_0i[i] 
			{
				ETAT[1].V = REC_[i + 1][suiv_list].V + E_i[i];
				ETAT[1].perf = REC_[i + 1][suiv_list].perf + (alpha * E_i[i]) + (beta*d_i[i]);
				ETAT[1].T = REC_[i + 1][suiv_list].T + d_i[i];
				ETAT[1].xx = 0;
				ETAT[1].succ = REC_[i + 1][suiv_list].succ;
				ETAT[1].jant = suiv_list;

			}
			else
			{
				ETAT[1].V = INFINI;
			}
			// si energie en(i + 1) + energie(usine->i+1) <= capa Reservoir
			//on est arrive en i+1 en venant de l'usine
			if ((REC_[i + 1][suiv_list].V + E_0i[i + 1] <= vmax) && (REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p <= TMAX))
			{
				ETAT[2].V = E_i0[i];
				ETAT[2].perf = REC_[i + 1][suiv_list].perf + (alpha * (E_i0[i] + E_0i[i + 1])) + (beta*(d_i0[i] + d_0i[i + 1] + p));
				ETAT[2].T = REC_[i + 1][suiv_list].T + d_i0[i] + d_0i[i + 1] + p;
				ETAT[2].xx = 1;

				ETAT[2].succ = REC_[i + 1][suiv_list].succ;
				ETAT[2].jant = suiv_list;
			}
			else
			{
				ETAT[2].V = INFINI;

			}
			for (int s = 1; s <= 2; s++)
			{

				if ((ETAT[s].V != INFINI) && (ETAT[s].T + TINF[i] <= TMAX))
				{
					if (REC[i] == -1)
					{
#ifdef VERIF_
						if (!verifEtatValide_PIPELINE_RECHARGE(ETAT[s]))
							stopProg("!! Heuristique::dyn_Recharge : on a cree un etat non valide");
#endif
						list_recharge.push_back(ETAT[s]);
						REC[i] = 0;
					}
					else
					{
						bool boo = true;
						int ind_perf = 2;
						it = list_recharge.begin();
						//parcourt la liste des etats, si on a un etat identique (meme V) deja present et meilleur on ecarte s (ind_perf  = 0)
						//si on a un etat identique moins bon => on le remplace (ind_perf = 1)

						while (it != list_recharge.end() && boo == true)
						{

							//si ETAT[s] est meilleur que it on remplace it par ETAT[s] :
							if (it->V >= ETAT[s].V)
							{
								if (ETAT[s].perf < it->perf)
								{
									ind_perf = 1;
									it->V = ETAT[s].V; //!!!!!!!!!!!!!!!!!!!!!ajouter le 10/ 06/ 2020
									it->perf = ETAT[s].perf;
									it->jant = ETAT[s].jant;
									it->xx = ETAT[s].xx;
									it->T = ETAT[s].T;
									boo = false;
								}
							}
							//si it est meilleur que ETAT[s] => ETAT[s] n'est pas ajoute
							if (it->V <= ETAT[s].V && it->perf <= ETAT[s].perf)
								ind_perf = 0;

							++it;
						}

						//on n a pas trouve d etat identique => on insere
						if (ind_perf == 2)
						{
#ifdef VERIF_
							if (!verifEtatValide_PIPELINE_RECHARGE(ETAT[s]))
								stopProg("!! Heuristique::dyn_Recharge : on a cree un etat non valide");
#endif
							list_recharge.push_back(ETAT[s]);
						}

					}


				}
			}

			suiv_list = suiv_list + 1;
		}


		K = 0;
		if (list_recharge.size() > Nombre_etats_maximal_recharge)Nombre_etats_maximal_recharge = list_recharge.size();
		for (it = list_recharge.begin(); it != list_recharge.end(); ++it)
		{
			//cout << "i=" << K << " : " << "(";
			REC_[i][K].V = it->V;
			REC_[i][K].perf = it->perf;
			Energie[i][REC_[i][K].V] = REC_[i][K].perf; //quantité d'energie minimal qu'il faut pour aller de i au dépôt avec un reservoir REC_[i][ind_tour].V

			REC_[i][K].jant = it->jant;
			REC_[i][K].xx = it->xx;
			REC_[i][K].T = it->T;
			//REC_[i][K].succ = K - 1;
			REC_[i][K].tr = 0;
			K = K + 1;
		}

		//cout << "Nombre d'etats " << list_recharge.size() << endl;
		//cout << endl;
		list_recharge.clear();
	}//fin pour principal


}

bool Heuristique::intervalles_recharge()
{
	//Correspondance entre ce code et l'article pipeline
	//S is the number of times a refueling transaction is performed;

	//St[s] is the station j such that the sth refueling transaction is performed between j and j + 1;
	//L[s] (correspond ici à u[s]) is the quantity of fuel which is loaded during the loading transaction s;
	//TInf[s] (correspond ici à A[s]) is the earliest time when it may start;
	//TSup[s] (correspond ici à B[s])is the latest possible date it may start;
	//Delta[s] (correspond ici à Deltaa[s]) is the minimal delay between the date when the sth refueling transaction may start and the date when the next one(s + 1) may start(or the end of the trip if s = S).

	//Primary-Refueling Procedure

	int j0 = -1;
	int PERF = INFINI;
	//calcul de j0
	int indice_list = REC[0];

	if (indice_list == -1)
		return false;//pas de solution

	if (indice_list > -1)//RN
	{
		while (REC_[0][indice_list].tr == 0)
		{
			if (REC_[0][indice_list].V <= v0 && REC_[0][indice_list].perf <= PERF)
			{
				j0 = indice_list;
				PERF = REC_[0][indice_list].perf;
			}

			indice_list = indice_list + 1;

		}


		//int TCurr = 0;
		int T0 = REC_[0][j0].T;
		int V0 = REC_[0][j0].V;
		int  q = j0;
		//int Taux;
		int Vaux;
		for (int j = 0; j <= Nb_stations; j++)
		{
			int T = REC_[j][q].T;
			//int  VVeh = REC_[j][q].V;
			//int W = REC_[j][q].perf;
			int x = REC_[j][q].xx;
			int Next = REC_[j][q].jant;
			int T1 = REC_[j + 1][Next].T;
			int V1Veh = REC_[j + 1][Next].V;
			//int W1 = REC_[j + 1][Next].perf;
			//int x1 = REC_[j + 1][Next].xx;
			//int Next1 = REC_[j + 1][Next].jant;
			if (x == 1)
			{
				if (S >= 1)
				{
					//Taux = A[S];
					Vaux = 0;
				}
				else
				{
					//Taux = 0;
					Vaux = v0 - V0;
				}

				S = S + 1;
				St[S] = j; //St[s]>-1 signifie qu'il y a une recharge entre le station  St[s] et la station St[s]+1 
				u[S] = V1Veh + E_0i[j + 1] - Vaux; //u[s]=quantité d'hydrogène rechargée à la recharge s
				A[S] = (T0 - T + d_i0[j]); //A[s]=date au plus tot de la recharge s (earliest time when the s^th refueling transaction may start;)
				B[S] = (TMAX - T1 - p - d_0i[j + 1]);//B[s]=date au plus tard de la recharge s

				//Je remplis le vecteur (SolutionPipelineRech) contenant la solution pipeline Recharge 
				SolutionPipelineRech[j].x = 1;
				SolutionPipelineRech[j].L = u[S];
				//Fin (SolutionPipelineRech)
			}
			q = Next;
		}
		if (St[S] + 1 == Nb_stations + 1)
		{
			temps_restant_st_S__0 = d_0i[St[S] + 1];
		}
		else
		{
			temps_restant_st_S__0 = d_0i[St[S] + 1];
			for (int i = St[S] + 1; i < Nb_stations; i++)temps_restant_st_S__0 += d_i[i];
			temps_restant_st_S__0 += d_i[Nb_stations];
		}
		for (int s = 1; s < S; s++)
		{
			Deltaa[s] = A[s + 1] - A[s]; //Deltaa[s]= nombre d'unités de temps (1) séparant la date au plus tot de la recharge s et date au plus tot de la recharge s+1
			//A[S+1]-A[S]=B[S-1]-B[S]
#ifdef VERIF_
			if (A[s + 1] - A[s] != B[s + 1] - B[s])
				stopProg("!! Heuristique::intervalles_recharge : Il y a un écart trop grand A[s + 1] - A[s] != B[s + 1] - B[s]");
#endif // VERIF_

		}


		//Reduced Refueling Procedure 

		for (int s = 1; s <= S; s++)
		{
			if (Deltaa[s] > 0)
			{
				float tempp7 = Deltaa[s] / p;
				BS[s] = static_cast<int>(ceil(tempp7) + DBL_EPSILON); //BS[s] = nombre de période séparant date au plus tot de la recharge s et date au plus tot de la recharge s+1
				//if (p == 1)BS[s] = static_cast<int>(ceil(tempp7) + DBL_EPSILON); //+ 1
			}
			muS[s] = u[s]; //muS[s]=quantité d'hydrogène rechargée à la recharge s
		}
		float tempp8 = (A[1] / p);
		mm[1] = static_cast<int>(ceil(tempp8) + DBL_EPSILON);
		for (int s = 1; s < S; s++)
		{
			mm[s + 1] = mm[s] + BS[s]; //mm[s]=période au plus tot de la recharge s

		}
		mm[0] = mm[1]; //Car dans l'algorithme de production pipeline dans VAL_TEST1, i+1-Gap-m_Rank n'est pas valide lorsque Rank=0
		float tempp9 = (TMAX - B[S]) / p;
		MM[S] = Nombre_de_periode - static_cast<int>((ceil(tempp9) + DBL_EPSILON));
		for (int s = S - 1; s > 0; s--)
		{
			MM[s] = MM[s + 1] - BS[s]; //MM[s] = période au plus tard de la recharge s
		}
		MM[S + 1] = TMAX + 1; //Car dans l'algorithme de production pipeline apres VAL_TEST2, la condition i+1<=M_Rank+1 n'est pas possible quand Rank+1=S  
	}

	return true;
}


void Heuristique::extraction_tour(string num_file, float alph, int bet)
{

	int nb_recharge = 0;
	std::string chemin{ "solutions_heuristique_pipeline/solution_recharge_" };
	std::string chemin_{ ".txt" };
	chemin = chemin + num_file + chemin_;

	ofstream monFlux_instances(chemin);

	cout << " Fonction extraction_tour" << endl;
	int j0 = -1;
	int PERF = INFINI;
	//calcul de j0
	int indice_list = REC[0];
	if (indice_list > -1) {
		while (REC_[0][indice_list].tr == 0)
		{
			//cout << "indice=" << indice_list << "V=" << REC_[0][indice_list].V << "perf=" << REC_[0][indice_list].perf << endl;
			if (REC_[0][indice_list].V <= v0 && REC_[0][indice_list].perf <= PERF)
			{
				j0 = indice_list;
				PERF = REC_[0][indice_list].perf;
			}

			indice_list = indice_list + 1;
			//indice_list = REC_[0][indice_list].succ;

		}
		cout_final_heuris_temps_energie = REC_[0][j0].perf;
		cout_final_heuris_temps = REC_[0][j0].T;
		//cout << j0 <<"fffff"<<endl;

		//écrire le chemin qui satisfait nos conditions dans un fichier csv

		int tamporaire;
		//fichier excel
		std::string chemin2{ "solutions_heuristique_pipeline/solution_rechargeex_" };
		std::string chemin2_{ ".csv" };
		chemin2 = chemin2 + num_file + chemin2_;

		ofstream monFlux_instances_excel(chemin2);
		monFlux_instances_excel << "stations" << ";" << "reservoir" << ";" << "dates_stations" << ";" << "décisions_recharge" << ";" << "coûts" << endl;


		int ind_tour = j0;//l'état qui satisfait nos conditions
		int i = 0;
		//list <int> tourne;
		tourne.push_front(0);
		int ind_stat = 0;
		int tempo = REC_[i][ind_tour].T; //on enregistre ceci pour avoir la date d'arrivée sur une station
		while (ind_tour != -1)
		{
			if (i == Nb_stations + 1)
			{
				tamporaire = 0;
			}
			else
			{
				tamporaire = i;
			}
			monFlux_instances_excel << tamporaire << ";" << REC_[i][ind_tour].V << ";" << tempo - REC_[i][ind_tour].T << ";" << REC_[i][ind_tour].xx << ";" << tempo - REC_[i][ind_tour].T << endl;

			//affichage dans le fichier
			monFlux_instances << "(";
			monFlux_instances << REC_[i][ind_tour].V << ", " << REC_[i][ind_tour].T << ", ";
			monFlux_instances << REC_[i][ind_tour].perf << ", ";
			//cout << REC_[i][K].succ << ", ";
			monFlux_instances << REC_[i][ind_tour].jant << ", ";
			monFlux_instances << REC_[i][ind_tour].xx;
			if (REC_[i][ind_tour].xx == 1)nb_recharge++;
			monFlux_instances << ")" << "-->";


			if (REC_[i][ind_tour].xx == 0) {
				ind_stat++;
				tourne.push_back(ind_stat);
			}
			else {
				if (REC_[i][ind_tour].xx == 1) {

					ind_stat++;
					tourne.push_back(-1);//-1 => detour
					tourne.push_back(ind_stat);
				}

			}
			ind_tour = REC_[i][ind_tour].jant;
			i = i + 1;
		}
		cout << "fin" << endl;
		monFlux_instances << "fin" << endl;
		monFlux_instances << "intervalle de chaque recharge" << endl;
		Q = S;
		for (int q = 1; q < Q + 1; q++)
		{

			B[q] = A[q] + TMAX - A[Q] - d_i0[Nb_stations] - d_0i[Nb_stations];
			monFlux_instances << "A[" << q << "]= " << A[q] << " et " << "B[" << q << "]= " << B[q] << " et " << "U[" << q << "] = " << u[q] << endl;
		}

		if (monFlux_instances)  //On teste si tout est OK
		{

			monFlux_instances << "La tournée est :" << endl;
			std::list<int>::iterator it_tr;
			for (it_tr = tourne.begin(); it_tr != tourne.end(); ++it_tr)
			{

				if (*it_tr == -1)
					monFlux_instances << "R" << "-->";
				else
					monFlux_instances << *it_tr << "-->";

			}
			monFlux_instances << "fin" << endl;
			//calcul du cout en temps de ma tournée

			std::list<int>::iterator ancien = tourne.begin();
			Cout_temps = 0;
			it_tr = tourne.begin();
			int temp_cpt = 0;
			++it_tr;
			while (it_tr != tourne.end())
			{
				//if ((*it_tr == 0 && *ancien == 0)||(*it_tr == Nb_stations+1 && *ancien == 0))
				//{
				//	Cout_temps = Cout_temps+1 ;
				//}
				//else
				//{
				//	if (*it_tr == 0 && *ancien != 0) Cout_temps = Cout_temps + d_i0[*ancien];
				//	
				//	if (*it_tr != 0 && *ancien == 0 && temp_cpt == 0)
				//	{
				//		Cout_temps = Cout_temps + d_i[*ancien];
				//	}
				//	else
				//	{
				//		if (*it_tr != 0 && *ancien == 0) Cout_temps = Cout_temps + d_0i[*it_tr];
				//	}
				//	if ((*it_tr != 0 && *ancien != 0)) Cout_temps = Cout_temps + d_i[*ancien];
				//}

				if (*it_tr == -1)//recharge
				{
					++it_tr;
					Cout_temps += d_i0[*ancien] + d_0i[*it_tr];
					ancien = it_tr;
				}
				else//pas de recharge
				{
					Cout_temps += d_i[*ancien];
				}
				//Cout_temps = Cout_temps + d[*ancien][*it_tr]; 

				ancien = it_tr;
				++it_tr;
				temp_cpt++;
			}
		}
		else
		{
			cout << "ERREUR: Impossible d'ouvrir le fichier." << endl;
		}
		Cout_temps = Cout_temps + nb_recharge * p;
#ifdef VERIF_
		if (cout_final_heuris_temps != Cout_temps)
			stopProg("!! Heuristique::extraction_tour : Erreur sur le calcul de la duree totale du parcourt avec attente");
		if (cout_final_heuris_temps > TMAX || Cout_temps > TMAX)
			stopProg("Heuristique::extraction_tour : Erreur : La duree de la tournee depasse le TMAX");
#endif // VERIF_


		cout << "La tournée est :" << endl;
		std::list<int>::iterator it_tr;
		for (it_tr = tourne.begin(); it_tr != tourne.end(); ++it_tr)
		{
			if (*it_tr == -1)
				cout << "R" << "-->";
			else
				cout << *it_tr << "-->";
		}
		cout << "fin" << endl;
		//cout << "le cout total (distance) avec attente est :" << cout_final_heuris_temps << endl;
		//cout << "le cout total (distance) sans attente , ni decalage est: " << Cout_temps << endl;
		//monFlux_instances << "le cout total(distance) sans attente, ni decalage est: " << Cout_temps << endl;
		monFlux_instances.close();
	}
}

/*bool Heuristique::dyn_Production_flexible(int num_file, int lamda, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_, int BSUP)
{
	//mm[1] = 2; MM[1] = 4; mm[2] = 7; MM[2] = 10; mm[3] = 13; MM[3] = 14; S = 3; BS[1] = 2; BS[2] = 2; muS[1] = 2; muS[2] = 2; muS[3] = 2;
	//Les procédés de filtrage sont les suivants :
	// VAL_TEST1 ---> Calcul l'estimation du coût anticipé pour satisfaire toutes les recharges (temps et cout de production). 
						//Ceci nous aidera à filtrer tous les états dont le cout + coût anticipé est supérieur au cout de la BSUP production
	// VAL_TEST_1 ---> Calcul la quantité d'hydrogène maximale qu'on peut produire avant TMAX (en commencant la production à la période actuelle)
	// VAL_TEST__1 ---> Calcul la quantité d'hydrogène maximale qu'on peut produire avant la dernière recharge (en commencant la production à la période actuelle)

	//Filtered-Production-DPS;
	bool filtrage_BSUP;
	float BSUP_prod_pi;
	if (BSUP>0)//(BSUP_prod_pipeline[num_file] > 0)
	{
		BSUP_prod_pi = BSUP;//BSUP_prod_pipeline[num_file];// + 97
	}
	else
	{
		BSUP_prod_pi = 2 * INFINI;
	}
	//int i = 0;
	N_PROD[0] = 1; // N_PROD[i] = Nombre d'états de la la liste PROD[i][]
	//Initialisation de l'état initial
	PROD[0].push_back({ 0, s0, 0, 0, 0, -1, -1, -1 });
	//Initialisation de la solution finale
	FINAL.Z = -1;
	FINAL.VTank = -1;
	FINAL.Rank = -1;
	FINAL.Gap = -1;
	FINAL.Wprod = INFINI;
	FINAL.Zant = -1;
	FINAL.Deltaant = -1;
	FINAL.Pred = -1;
	for (int temp_i = 1; temp_i <= Nombre_de_periode; temp_i++)
	{
		N_PROD[temp_i] = 0;
	}

	for (int i = 0; i <= Nombre_de_periode - 1; i++)
	{
		if (N_PROD[i] > Nombre_etats_maximal_prod) Nombre_etats_maximal_prod = N_PROD[i];
		for (int q = 0; q < N_PROD[i]; q++)
		{
			//cout <<"instance "<< num_file  << "Periode " << i << "Numero etat " << q << endl;
			//Concernant la décision (z,delta)=(0,0)
			pair<int, int> DEC1(0, 0);
			Etat_prod E1;
			int W1, EVAL1;
			E1.Z = 0, E1.VTank = PROD[i][q].VTank, E1.Rank = PROD[i][q].Rank;
			E1.Gap = PROD[i][q].Gap + 1;
			if (PROD[i][q].Rank == S)E1.Gap = PROD[i][q].Gap;// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!modifé 19 / 05 / 2022
			if (PROD[i][q].Rank <= S - 1)
			{
				W1 = PROD[i][q].Wprod + lamda; //car ne rien faire coute lambda
			}
			else
			{
				W1 = PROD[i][q].Wprod;
			}
			int Aproduire = 0;
			for (int itemp = PROD[i][q].Rank + 1; itemp <= S; itemp++)
				Aproduire += muS[itemp];
			int temp_apr = max(Aproduire + s0 - PROD[i][q].VTank, 0);
			//int temp_ecoule_depuis_dernier_recharge1 =  max(0,i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]);
			//int VAL_TEST1 = VAL[i + 1][0][temp_apr] + lamda * (mm[S] - temp_ecoule_depuis_dernier_recharge1);
			//if (mm[S] - E1.Gap - mm[PROD[i][q].Rank] < 0)
			//	stopProg("Heuristique::dyn_Production_flexible : Erreur dec 1");
			int VAL_TEST1 = VAL[i + 1][0][temp_apr] + lamda * (mm[S] - E1.Gap - mm[PROD[i][q].Rank]);// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!modifé 19 / 05 / 2022
			//mm[s] + (i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]) = On finit la tournée à la date de la dernière recharge(mm[s]) + les décalages induits par les recharges précédentes.
			int VAL_TEST_1 = PROD[i][q].VTank + q__[i + 1] - Aproduire + s0;// Prod-Max[i+1]= q__[i + 1], 
			//VAL_TEST_1 est l'énergie en plus(entre i+1 et Nombre de période) après avoir satisfait les Rank premières demandes en hydrogènes
			int VAL_TEST__1 = PROD[i][q].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
			//VAL_TEST__1 est l'énergie en plus(entre i+1 et MM[S]) après avoir satisfait les Rank premières demandes en hydrogènes
			filtrage_BSUP = (VAL_TEST1 + W1 <= BSUP_prod_pi);
			//filtrage_BSUP = true;
			if (PIPELINE_SANS_filtrage_BSUP_)
				filtrage_BSUP = true;
			bool filtrage_1 = filtrage_BSUP && (VAL_TEST_1 >= 0) && (VAL_TEST__1 >= 0);
			if (PIPELINE_SANS_filtrage_)
				filtrage_1 = true;
			if ((i + 1 <= MM[PROD[i][q].Rank + 1]) && filtrage_1)
			{
				//Prod-Bellman-Update
				Prod_Bellman_Update(i, q, DEC1, E1, W1,Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);

			}

			//Concernant la décision (z,delta)=(1,0)
			pair<int, int> DEC2(1, 0); //Production d'hydrogene par la micro-usine a la periode [i,i+1]
			Etat_prod E2;
			int W2, EVAL2;
			E2.Z = 1, E2.VTank = PROD[i][q].VTank + rendement[i], E2.Rank = PROD[i][q].Rank;
			E2.Gap = PROD[i][q].Gap + 1;
			if (PROD[i][q].Rank == S)E2.Gap = PROD[i][q].Gap; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!modifié 19 / 05 / 2022
			if (PROD[i][q].Rank <= S - 1)
			{
				W2 = PROD[i][q].Wprod + Cf * (1 - PROD[i][q].Z) + coutv[i] + lamda;
			}
			else
			{
				W2 = PROD[i][q].Wprod + Cf * (1 - PROD[i][q].Z) + coutv[i];
			}
			int temp_aprod = max(Aproduire + s0 - PROD[i][q].VTank - rendement[i], 0);
			//int temp_ecoule_depuis_dernier_recharge2 = max(0, i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]);//!!!!!!!!!!!! enlever max
			//int VAL_TEST2 = VAL[i + 1][1][temp_aprod] + lamda * (mm[S] - temp_ecoule_depuis_dernier_recharge2);
			//if (mm[S] - E2.Gap - mm[PROD[i][q].Rank] < 0)
			//	stopProg("Heuristique::dyn_Production_flexible : Erreur dec 2");
			int VAL_TEST2 = VAL[i + 1][1][temp_aprod] + lamda * (mm[S] - E2.Gap - mm[PROD[i][q].Rank]);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!modifié 19 05 2022
			bool filtrage_2 = (VAL_TEST2 + W2 <= BSUP_prod_pi);
			//filtrage_2 = true;
			if (PIPELINE_SANS_filtrage_BSUP_)
				filtrage_2 = true;
			if (PIPELINE_SANS_filtrage_)
				filtrage_2 = true;
			if ((PROD[i][q].VTank + rendement[i] <= smax) && (i + 1 <= MM[PROD[i][q].Rank + 1]) && filtrage_2)
			{
				//Prod-Bellman-Update
				Prod_Bellman_Update(i, q, DEC2, E2, W2, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);
			}

			//Concernant la décision (z,delta)=(0,1)

			if (PROD[i][q].Rank <= S - 1)
			{
				pair<int, int> DEC3(0, 1);
				Etat_prod E3;
				int W3, EVAL3;
				E3.Z = 0; E3.VTank = PROD[i][q].VTank - muS[PROD[i][q].Rank + 1]; E3.Rank = PROD[i][q].Rank + 1;
				E3.Gap = 1; //Lorsqu'on fait une recharge le temps écoulé après la dernière recharge est reinitialisé à 1
				if (PROD[i][q].Rank + 1 == S)E3.Gap = 0;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ajouté 19/05/2022
				if (PROD[i][q].Rank + 1 <= S - 1)
				{
					W3 = PROD[i][q].Wprod + lamda;
				}
				else
				{
					W3 = PROD[i][q].Wprod;
				}
				int temp_aprodd = max(Aproduire + s0 - PROD[i][q].VTank, 0);
				//if (mm[S] - mm[PROD[i][q].Rank + 1] - E3.Gap < 0)
				//	stopProg("Heuristique::dyn_Production_flexible : Erreur dec 3");
				//int VAL_TEST3 = VAL[i + 1][0][temp_aprodd] + lamda * (mm[S] + i - mm[PROD[i][q].Rank + 1] - E3.Gap);// + i
				int VAL_TEST3 = VAL[i + 1][0][temp_aprodd] + lamda * (mm[S] - mm[PROD[i][q].Rank + 1] - E3.Gap);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!modifé 19 05 2022
				int VAL_TEST_3 = PROD[i][q].VTank + q__[i + 1] - Aproduire + s0;
				int VAL_TEST__3 = PROD[i][q].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
				filtrage_BSUP = (VAL_TEST3 + W3 <= BSUP_prod_pi);
				filtrage_BSUP = true;
				if (PIPELINE_SANS_filtrage_BSUP_)
					filtrage_BSUP = true;
				bool filtrage_3 = filtrage_BSUP && (VAL_TEST_3 >= 0) && (VAL_TEST__3 >= 0);
				if (PIPELINE_SANS_filtrage_)
					filtrage_3 = true;
				if ((PROD[i][q].VTank - muS[PROD[i][q].Rank + 1] >= 0) && (MM[PROD[i][q].Rank + 1] >= i)
					&& (i >= mm[PROD[i][q].Rank + 1]) && (PROD[i][q].Gap >= BS[PROD[i][q].Rank]) && filtrage_3)
				{
					////Prod-Bellman-Update
					Prod_Bellman_Update(i, q, DEC3, E3, W3, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);
				}
			}
		}
	}
	//Je remplis le vecteur (SolutionPipelineProd) contenant la solution pipeline Production 
	Etat_prod temp_Eta_prod = FINAL;
	int i = FINAL_TIME;
	while (temp_Eta_prod.Pred != -1)
	{
		SolutionPipelineProd[i - 1].z = temp_Eta_prod.Z;
		//if(temp_Eta_prod.Z==1)
		SolutionPipelineProd[i - 1].y = temp_Eta_prod.Z*(1 - PROD[i - 1][temp_Eta_prod.Pred].Z);
		SolutionPipelineProd[i - 1].delt = temp_Eta_prod.Deltaant;
		temp_Eta_prod = PROD[i - 1][temp_Eta_prod.Pred];
#ifdef VERIF_
		if (SolutionPipelineProd[i - 1].z && SolutionPipelineProd[i - 1].delt)
			stopProg("Heuristique::dyn_Production_flexible : Erreur: Il y a production et recharge simultanement");
#endif // VERIF_

		i -= 1;
	}

	return FINAL.Wprod < INFINI;

*/

bool Heuristique::dyn_Production_flexible(int num_file, int lamda, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_, int BSUP)
{
	//mm[1] = 2; MM[1] = 4; mm[2] = 7; MM[2] = 10; mm[3] = 13; MM[3] = 14; S = 3; BS[1] = 2; BS[2] = 2; muS[1] = 2; muS[2] = 2; muS[3] = 2;
	//Les procédés de filtrage sont les suivants :
	// VAL_TEST1 ---> Calcul l'estimation du coût anticipé pour satisfaire toutes les recharges (temps et cout de production). 
						//Ceci nous aidera à filtrer tous les états dont le cout + coût anticipé est supérieur au cout de la BSUP production
	// VAL_TEST_1 ---> Calcul la quantité d'hydrogène maximale qu'on peut produire avant TMAX (en commencant la production à la période actuelle)
	// VAL_TEST__1 ---> Calcul la quantité d'hydrogène maximale qu'on peut produire avant la dernière recharge (en commencant la production à la période actuelle)

	//Filtered-Production-DPS;
	 //start;
	//if (Filtrage_Heuristique_Pipe_ == false)
	//{
	//======================================Permet de mettre le temps limit====================
	 auto start = std::chrono::system_clock::now();
	//}
	//======================================
	bool filtrage_BSUP;
	float BSUP_prod_pi;
	if (BSUP > 0)//(BSUP_prod_pipeline[num_file] > 0)
	{
		//23/05/2022 : il faut enlever ceci à la BSup -( p + temps_restant_st_S__0)
		BSUP_prod_pi = BSUP;// -(p + temps_restant_st_S__0);//BSUP_prod_pipeline[num_file];// + 97
	}
	else
	{
		BSUP_prod_pi = 2 * INFINI;
	}
	//int i = 0;
	N_PROD[0] = 1; // N_PROD[i] = Nombre d'états de la la liste PROD[i][]
	//Initialisation de l'état initial
	PROD[0].push_back({ 0, s0, 0, 0, 0, -1, -1, -1 });
	//Initialisation de la solution finale
	FINAL.Z = -1;
	FINAL.VTank = -1;
	FINAL.Rank = -1;
	FINAL.Gap = -1;
	FINAL.Wprod = INFINI;
	FINAL.Zant = -1;
	FINAL.Deltaant = -1;
	FINAL.Pred = -1;
	for (int temp_i = 1; temp_i <= Nombre_de_periode; temp_i++)
	{
		N_PROD[temp_i] = 0;
	}

	for (int i = 0; i <= Nombre_de_periode - 1; i++)
	{
		//======================================Permet de mettre le temps limit====================
		if (Filtrage_Heuristique_Pipe_ == false)
		{
			auto end = std::chrono::system_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			double CpuT = static_cast<double> (elapsed.count()) / 1000;
			if (CpuT > TEMPS_LIMIT)
			{
				cout << "Time Limit";
				break;

			}
		}
		//==========================================================
		if (N_PROD[i] > Nombre_etats_maximal_prod) Nombre_etats_maximal_prod = N_PROD[i];
		for (int q = 0; q < N_PROD[i]; q++)
		{
			//======================================Permet de mettre le temps limit====================
			if (Filtrage_Heuristique_Pipe_ == false)
			{
				auto end = std::chrono::system_clock::now();
				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
				double CpuT = static_cast<double> (elapsed.count()) / 1000;
				if (CpuT > TEMPS_LIMIT)
				{
					cout << "Time Limit";
					break;

				}
			}
			//==========================================================

			//Etat_prod e = PROD[i][q];//e.Deltaant== e.Gap== e.Rank== e.VTank== e.Wprod == e.Z==e.Zant==
			//cout <<"instance "<< num_file  << "Periode " << i << "Numero etat " << q << endl;
			//Concernant la décision (z,delta)=(0,0)
			pair<int, int> DEC1(0, 0);
			Etat_prod E1;
			int W1;// EVAL1;
			E1.Z = 0, E1.VTank = PROD[i][q].VTank, E1.Rank = PROD[i][q].Rank;
			E1.Gap = PROD[i][q].Gap + 1;
			if (PROD[i][q].Rank == S)E1.Gap = 0;
			if (PROD[i][q].Rank <= S - 1)
			{
				W1 = PROD[i][q].Wprod + lamda; //car ne rien faire coute lambda
			}
			else
			{
				W1 = PROD[i][q].Wprod;
			}
			int Aproduire = 0;
			for (int itemp = PROD[i][q].Rank + 1; itemp <= S; itemp++)
				Aproduire += muS[itemp];
			int temp_apr = max(Aproduire + s0 - PROD[i][q].VTank, 0);
			//int temp_ecoule_depuis_dernier_recharge1 = max(0, i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]);
			int temp_ecoule_depuis_dernier_recharge1 = -E1.Gap - mm[E1.Rank];
			int VAL_TEST1 = VAL[i + 1][0][temp_apr] + lamda * (mm[S] + temp_ecoule_depuis_dernier_recharge1);
			//mm[s] + (i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]) = On finit la tournée à la date de la dernière recharge(mm[s]) + les décalages induits par les recharges précédentes.
			int VAL_TEST_1 = PROD[i][q].VTank + q__[i + 1] - Aproduire + s0;// Prod-Max[i+1]= q__[i + 1], 
			//VAL_TEST_1 est l'énergie en plus(entre i+1 et Nombre de période) après avoir satisfait les Rank premières demandes en hydrogènes
			int VAL_TEST__1 = 0;
			if (E1.Rank < S)//20 05 2022 : On réalise ce test s'il reste des recharges à faire
				VAL_TEST__1 = PROD[i][q].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
			//VAL_TEST__1 est l'énergie en plus(entre i+1 et MM[S]) après avoir satisfait les Rank premières demandes en hydrogènes
			filtrage_BSUP = (VAL_TEST1 + W1 <= BSUP_prod_pi);
			if (PIPELINE_SANS_filtrage_BSUP_)
				filtrage_BSUP = true;
			bool filtrage_1 = filtrage_BSUP && (VAL_TEST_1 >= 0) && (VAL_TEST__1 >= 0);
			if (PIPELINE_SANS_filtrage_)
				filtrage_1 = true;
			if ((i + 1 <= MM[PROD[i][q].Rank + 1]) && filtrage_1)
			{
				//Prod-Bellman-Update
				Prod_Bellman_Update(i, q, DEC1, E1, W1, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);

			}

			//Concernant la décision (z,delta)=(1,0)
			pair<int, int> DEC2(1, 0); //Production d'hydrogene par la micro-usine a la periode [i,i+1]
			Etat_prod E2;
			int W2;// EVAL2;
			E2.Z = 1, E2.VTank = PROD[i][q].VTank + rendement[i], E2.Rank = PROD[i][q].Rank;
			E2.Gap = PROD[i][q].Gap + 1;
			if (PROD[i][q].Rank == S)E2.Gap = 0;
			if (PROD[i][q].Rank <= S - 1)
			{
				W2 = PROD[i][q].Wprod + Cf * (1 - PROD[i][q].Z) + coutv[i] + lamda;
			}
			else
			{
				W2 = PROD[i][q].Wprod + Cf * (1 - PROD[i][q].Z) + coutv[i];
			}
			int temp_aprod = max(Aproduire + s0 - PROD[i][q].VTank - rendement[i], 0);
			//int temp_ecoule_depuis_dernier_recharge2 = max(0, i + 1 - PROD[i][q].Gap - mm[PROD[i][q].Rank]);
			int temp_ecoule_depuis_dernier_recharge2 = -E2.Gap - mm[E2.Rank];
			int VAL_TEST2 = VAL[i + 1][1][temp_aprod] + lamda * (mm[S] + temp_ecoule_depuis_dernier_recharge2);
			bool filtrage_2 = (VAL_TEST2 + W2 <= BSUP_prod_pi);
			if (PIPELINE_SANS_filtrage_BSUP_)
				filtrage_2 = true;
			if (PIPELINE_SANS_filtrage_)
				filtrage_2 = true;
			if ((PROD[i][q].VTank + rendement[i] <= smax) && (i + 1 <= MM[PROD[i][q].Rank + 1]) && filtrage_2)
			{
				//Prod-Bellman-Update
				Prod_Bellman_Update(i, q, DEC2, E2, W2, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);
			}

			//Concernant la décision (z,delta)=(0,1)

			if (PROD[i][q].Rank <= S - 1)
			{
				pair<int, int> DEC3(0, 1);
				Etat_prod E3;
				int W3;// EVAL3;
				E3.Z = 0; E3.VTank = PROD[i][q].VTank - muS[PROD[i][q].Rank + 1]; E3.Rank = PROD[i][q].Rank + 1;
				E3.Gap = 1; //Lorsqu'on fait une recharge le temps écoulé après la dernière recharge est reinitialisé à 1
				if (PROD[i][q].Rank + 1 <= S - 1)
				{
					W3 = PROD[i][q].Wprod + lamda;
				}
				else
				{
					W3 = PROD[i][q].Wprod;
				}
				int Aproduire = 0;//20 05 2022 : Prochaine recharge à Rank +2 donc Aproduire doit être recalculé
				for (int itemp = E3.Rank + 1; itemp <= S; itemp++)
					Aproduire += muS[itemp];
				int temp_aprodd = max(Aproduire + s0 - PROD[i][q].VTank, 0);
				int VAL_TEST3 = VAL[i + 1][0][temp_aprodd] + lamda * (mm[S] - E3.Gap - mm[E3.Rank]);// i - mm[PROD[i][q].Rank + 1]);
				int VAL_TEST_3 = PROD[i][q].VTank + q__[i + 1] - Aproduire + s0;
				int VAL_TEST__3 = 0;
				if (E3.Rank < S)//20 05 2022 : On réalise ce test s'il reste des recharges à faire
					VAL_TEST__3 = PROD[i][q].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
				filtrage_BSUP = (VAL_TEST3 + W3 <= BSUP_prod_pi);
				if (PIPELINE_SANS_filtrage_BSUP_)
					filtrage_BSUP = true;
				bool filtrage_3 = filtrage_BSUP && (VAL_TEST_3 >= 0) && (VAL_TEST__3 >= 0);
				if (PIPELINE_SANS_filtrage_)
					filtrage_3 = true;
				if ((PROD[i][q].VTank - muS[PROD[i][q].Rank + 1] >= 0) && (MM[PROD[i][q].Rank + 1] >= i)
					&& (i >= mm[PROD[i][q].Rank + 1]) && (PROD[i][q].Gap >= BS[PROD[i][q].Rank]) && filtrage_3)
				{
					////Prod-Bellman-Update
					Prod_Bellman_Update(i, q, DEC3, E3, W3, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_);
				}
			}
		}
	}
	//Je remplis le vecteur (SolutionPipelineProd) contenant la solution pipeline Production 
	Etat_prod temp_Eta_prod = FINAL;
	int i = FINAL_TIME;
	while (temp_Eta_prod.Pred != -1)
	{
		SolutionPipelineProd[i - 1].z = temp_Eta_prod.Z;
		//if(temp_Eta_prod.Z==1)
		SolutionPipelineProd[i - 1].y = temp_Eta_prod.Z*(1 - PROD[i - 1][temp_Eta_prod.Pred].Z);
		SolutionPipelineProd[i - 1].delt = temp_Eta_prod.Deltaant;
		temp_Eta_prod = PROD[i - 1][temp_Eta_prod.Pred];
#ifdef VERIF_
		if (SolutionPipelineProd[i - 1].z && SolutionPipelineProd[i - 1].delt)
			stopProg("Heuristique::dyn_Production_flexible : Erreur: Il y a production et recharge simultanement");
#endif // VERIF_

		i -= 1;
	}

	return FINAL.Wprod < INFINI;

}


void Heuristique::Prod_Bellman_Update(int i, int q, pair<int, int> DEC, Etat_prod E, int W, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_)
{
#ifdef VERIF_
	E.Wprod = W;
	E.Zant = DEC.first;
	E.Deltaant = DEC.second;
	if (!verifEtatValide_PIPELINE(E))
		stopProg("!! Heuristique::Prod_Bellman_Update : on a cree un etat non valide");
#endif
	int Not_stop = true;
	if ((E.VTank >= s0) && (E.Rank >= S))
	{
		if (W <= FINAL.Wprod)
		{

			FINAL.Z = E.Z;
			FINAL.VTank = E.VTank;
			FINAL.Rank = E.Rank;
			FINAL.Gap = E.Gap;
			FINAL.Wprod = W;
			FINAL.Zant = DEC.first;
			FINAL.Deltaant = DEC.second;
			FINAL.Pred = q;
			FINAL_TIME = i + 1;
#ifdef VERIF_

			if ((W == FINAL.Wprod) && (FINAL.Gap != E.Gap || FINAL.Rank != E.Rank ||
				FINAL.VTank != E.VTank || FINAL.Z != E.Z))
			{
				stopProg("!!!!!!!!!!!!!!!!!!!!!!! Heuristique::Prod_Bellman_Update : pipeline FINAL : On a plusieurs solutions finales possibles au meme cout");

			}
#endif
		}

	}
	else
	{

		int q1 = 0;
		while ((q1 < N_PROD[i + 1]) && (Not_stop))
		{
#ifdef VERIF_
			if ((W == PROD[i + 1][q1].Wprod) && (PROD[i + 1][q1].Gap == E.Gap && PROD[i + 1][q1].Rank == E.Rank &&
				PROD[i + 1][q1].VTank == E.VTank && PROD[i + 1][q1].Z == E.Z) &&
				(PROD[i][PROD[i + 1][q1].Pred].Gap != PROD[i][q].Gap || PROD[i][PROD[i + 1][q1].Pred].Rank != PROD[i][q].Rank ||
					PROD[i][PROD[i + 1][q1].Pred].VTank != PROD[i][q].VTank || PROD[i][PROD[i + 1][q1].Pred].Z != PROD[i][q].Z))
			{
				//stopProg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Heuristique::Prod_Bellman_Update pipeline : On a arrive au même état mais les prédécésseurs sont différents. Quel chemin choisir ? On prend celui dont le prédécesseur a le plus grand gap, le plus grand reservoir, le plus grand rank et le plus grand état de l'usine ?  ! ");

			}
#endif

			if (PROD[i + 1][q1].Gap == E.Gap && PROD[i + 1][q1].Rank == E.Rank &&
				PROD[i + 1][q1].VTank == E.VTank && PROD[i + 1][q1].Z == E.Z)
			{
				Not_stop = false;

			}
			else
			{
				q1 += 1;
			}

		}
		if (Not_stop)
		{
			if (Filtrage_exacte_pipe_)
			{
				N_PROD[i + 1] += 1;
				E.Wprod = W;
				E.Zant = DEC.first;
				E.Deltaant = DEC.second;
				E.Pred = q;
				PROD[i + 1].push_back(E);
			} // Filtrage_exacte_pipe_

			//Le filtrage heuristique consiste à checher le premier état f1 dont le cout est w1 tel que E.Z == PROD[i+1][it_etat].Z && abs(E.VTank - PROD[i + 1][it_etat].VTank) <= (smax / K_FILTRAGE_HEURIS_Pipe_) && abs(E.Rank == PROD[i + 1][it_etat].Rank)
			if (Filtrage_Heuristique_Pipe_)
			{
				int it_etat = 0;
				bool stop = true;
				//int K = 6;//paramètre K utilisé pour réaliser le filtrage heuristique
				while (it_etat < N_PROD[i + 1] && stop)
				{
					if (E.Z == PROD[i + 1][it_etat].Z && abs(E.VTank - PROD[i + 1][it_etat].VTank) <= (smax / K_FILTRAGE_HEURIS_Pipe_) && abs(E.Rank == PROD[i + 1][it_etat].Rank))//il faut ajouter la contrainte sur les T
					{
						if (W < PROD[i + 1][it_etat].Wprod)
						{
							PROD[i + 1][it_etat].Deltaant = DEC.second;
							PROD[i + 1][it_etat].Gap = E.Gap;
							PROD[i + 1][it_etat].Pred = q;
							PROD[i + 1][it_etat].Rank = E.Rank;
							PROD[i + 1][it_etat].VTank = E.VTank;
							PROD[i + 1][it_etat].Wprod = W;
							PROD[i + 1][it_etat].Z = E.Z;
							PROD[i + 1][it_etat].Zant = DEC.first;
							stop = false;
						}
					}
					it_etat++;

				}
				if (it_etat == N_PROD[i + 1])
				{
					N_PROD[i + 1] += 1;
					E.Wprod = W;
					E.Zant = DEC.first;
					E.Deltaant = DEC.second;
					E.Pred = q;
					PROD[i + 1].push_back(E);
				}

			}// Filtrage_Heuristique_Pipe_

		}
		else
		{
			int WAux = PROD[i + 1][q1].Wprod;
			if (W < WAux)
			{
				PROD[i + 1][q1].Z = E.Z;
				PROD[i + 1][q1].VTank = E.VTank;
				PROD[i + 1][q1].Rank = E.Rank;
				PROD[i + 1][q1].Gap = E.Gap;
				PROD[i + 1][q1].Wprod = W;
				PROD[i + 1][q1].Zant = DEC.first;
				PROD[i + 1][q1].Deltaant = DEC.second;
				PROD[i + 1][q1].Pred = q;
			}
			else
			{
				if (W == WAux)
				{
					//stopProg("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Heuristique::Prod_Bellman_Update pipeline : On a arrive au même état mais les prédécésseurs sont différents. Quel chemin choisir ? On prend celui dont le prédécesseur a le plus grand gap, le plus grand reservoir, le plus grand rank et le plus grand état de l'usine ?  ! ");

					/*if (PROD[i + 1][PROD[i + 1][q1].Pred].Rank < PROD[i][q].Rank)
					if (PROD[i + 1][PROD[i + 1][q1].Pred].Gap < PROD[i][q].Gap )
					if (PROD[i + 1][PROD[i + 1][q1].Pred].VTank < PROD[i][q].VTank)
					if(PROD[i + 1][PROD[i + 1][q1].Pred].Z > PROD[i][q].Z)*/
					if (PROD[i][PROD[i + 1][q1].Pred].Gap < PROD[i][q].Gap)
					{
						//PROD[i][PROD[i + 1][q1].Pred].Gap est le prédécesseur de PROD[i + 1][q1]
						//PROD[i][q].Gap est le prédécesseur de E
						PROD[i + 1][q1].Pred = q;
					}
				}
			}
		}




	}

}

void Heuristique::BSUP_Production_flexible(int lamda, string num_file)
{

	//Greedy - Production - DPS

	//initialaiation
	int BSUP_Prod = INFINI;
	int i = 0;
	ETATT[0].Z = 0, ETATT[0].VTank = s0, ETATT[0].Rank = 1, ETATT[0].Gap = 0;
	ETATT[0].Wprod = 0;
	//boucle principale
	bool Not_Failure = true, Not_success = true;
	while (Not_Failure && Not_success)
	{
		//Concernant la décision (z,delta)=(0,0)
		pair<int, int> DEC1(0, 0);
		Etat_prod E1;
		E1.Zant = 0; E1.Deltaant = 0;
		int W1, EVAL1;
		E1.Z = 0, E1.VTank = ETATT[i].VTank, E1.Rank = ETATT[i].Rank;
		E1.Gap = ETATT[i].Gap + 1;
		if (ETATT[i].Rank == S)E1.Gap = 0;
		if (ETATT[i].Rank <= S - 1)
		{
			W1 = ETATT[i].Wprod + lamda;
		}
		else
		{
			W1 = ETATT[i].Wprod;
		}
		E1.Wprod = W1;
		int Aproduire = 0;
		for (int itemp = ETATT[i].Rank + 1; itemp <= S; itemp++)
			Aproduire += muS[itemp];
		int temp_apr = max(Aproduire + s0 - ETATT[i].VTank, 0);
		int temp_ecoule_depuis_dernier_recharge1 = max(0, i + 1 - ETATT[i].Gap - mm[ETATT[i].Rank]);
		int VAL_TEST1 = VAL[i + 1][0][temp_apr] + lamda * (mm[S] + temp_ecoule_depuis_dernier_recharge1);
		//mm[s] + (i + 1 - ETATT[i].Gap - mm[ETATT[i].Rank]) = On finit la tournée à la date de la dernière recharge(mm[s]) + les décalages induits par les recharges précédentes.
		int VAL_TEST_1 = ETATT[i].VTank + q__[i + 1] - Aproduire + s0;// Prod - Max[i+1]= q__[i + 1]
		int VAL_TEST__1 = ETATT[i].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
		if ((i + 1 <= MM[ETATT[i].Rank + 1]) && (VAL_TEST_1 >= 0) && (VAL_TEST__1 >= 0))
		{
			EVAL1 = VAL_TEST1 + W1;
		}
		else
		{
			EVAL1 = INFINI;
		}

		//Concernant la décision (z,delta)=(1,0)
		pair<int, int> DEC2(1, 0);
		Etat_prod E2;
		E2.Zant = 1; E2.Deltaant = 0;
		int W2, EVAL2;
		E2.Z = 1, E2.VTank = ETATT[i].VTank + rendement[i], E2.Rank = ETATT[i].Rank;
		E2.Gap = ETATT[i].Gap + 1;
		if (ETATT[i].Rank == S)E2.Gap = 0;
		if (ETATT[i].Rank <= S - 1)
		{
			W2 = ETATT[i].Wprod + Cf * (1 - ETATT[i].Z) + coutv[i] + lamda;
		}
		else
		{
			W2 = ETATT[i].Wprod + Cf * (1 - ETATT[i].Z) + coutv[i];
		}
		E2.Wprod = W2;
		int temp_aprod = max(Aproduire + s0 - ETATT[i].VTank - rendement[i], 0);
		int temp_ecoule_depuis_dernier_recharge2 = max(0, i + 1 - ETATT[i].Gap - mm[ETATT[i].Rank]);
		int VAL_TEST2 = VAL[i + 1][1][temp_aprod] + lamda * (mm[S] + temp_ecoule_depuis_dernier_recharge2);
		if ((ETATT[i].VTank + rendement[i] <= smax) && (i + 1 <= MM[ETATT[i].Rank + 1]))
		{
			EVAL2 = VAL_TEST2 + W2;
		}
		else
		{
			EVAL2 = INFINI;
		}
		int EVAL3 = INFINI;
		pair<int, int> DEC3(0, 1);
		Etat_prod E3;
		E3.Z = -1; E3.VTank = -1; E3.Rank = -1;E3.Gap = -1;
		E3.Zant = 0; E3.Deltaant = 1;
		int W3 = INFINI;
		//Concernant la décision (z,delta)=(0,1)
		if (ETATT[i].Rank + 1 <= S)
		{
			
			E3.Z = 0; E3.VTank = ETATT[i].VTank - muS[ETATT[i].Rank + 1]; E3.Rank = ETATT[i].Rank + 1;
			E3.Gap = 1; //Lorsqu'on fait une recharge le temps écoulé après la dernière recharge est reinitialisé à 1
			if (ETATT[i].Rank + 1 <= S - 1)
			{
				W3 = ETATT[i].Wprod + lamda;
			}
			else
			{
				W3 = ETATT[i].Wprod;
			}
			E3.Wprod = W3;
			int temp_aprodd = max(Aproduire + s0 - ETATT[i].VTank, 0);
			int VAL_TEST3 = VAL[i + 1][0][temp_aprodd] + lamda * (mm[S] + i - mm[ETATT[i].Rank + 1]);
			int VAL_TEST_3 = ETATT[i].VTank + q__[i + 1] - Aproduire + s0;
			int VAL_TEST__3 = ETATT[i].VTank + q__[i + 1] - q__[MM[S]] - Aproduire;
			if ((ETATT[i].VTank - muS[ETATT[i].Rank + 1] >= 0) && (MM[ETATT[i].Rank + 1] >= i)
				&& (i >= mm[ETATT[i].Rank + 1]) && (ETATT[i].Gap >= BS[ETATT[i].Rank]) && (VAL_TEST_3 >= 0) && (VAL_TEST__3 >= 0))
			{
				EVAL3 = VAL_TEST3 + W3;
			}
			else
			{
				EVAL3 = INFINI;
			}
		}
#ifdef VERIF_
		if (EVAL1 != INFINI && !verifEtatValide_PIPELINE(E1))
			stopProg("!! Heuristique::BSUP_Production_flexible : on a cree un etat non valide E1");
		if (EVAL2 != INFINI && !verifEtatValide_PIPELINE(E2))
			stopProg("!! Heuristique::BSUP_Production_flexible : on a cree un etat non valide E2");
		if (EVAL3 != INFINI && !verifEtatValide_PIPELINE(E3))
			stopProg("!! Heuristique::BSUP_Production_flexible : on a cree un etat non valide E3");
#endif
		int EVAL_min = min(EVAL1, EVAL2);
		int EVAL = min(EVAL_min, EVAL3);
		if (EVAL == INFINI)
		{
			Not_Failure = false;
			cout << "Pipeline : Pas de solution BSUP pour la production " << endl;
			//stopProg(" Heuristique::BSUP_Production_flexible : pas de solution (liste vide)");
		}
		else
		{
			if (EVAL == EVAL1)
			{
				DEC_Prod[i + 1] = DEC1;
				ETATT[i + 1].Wprod = W1;
				ETATT[i + 1].Z = E1.Z;
				ETATT[i + 1].VTank = E1.VTank;
				ETATT[i + 1].Rank = E1.Rank;
				ETATT[i + 1].Gap = E1.Gap;
			}
			else
			{
				if (EVAL == EVAL2)
				{
					DEC_Prod[i + 1] = DEC2;
					ETATT[i + 1].Wprod = W2;
					ETATT[i + 1].Z = E2.Z;
					ETATT[i + 1].VTank = E2.VTank;
					ETATT[i + 1].Rank = E2.Rank;
					ETATT[i + 1].Gap = E2.Gap;

				}
				else
				{
					if (EVAL == EVAL3)
					{
						DEC_Prod[i + 1] = DEC3;
						ETATT[i + 1].Wprod = W3;
						ETATT[i + 1].Z = E3.Z;
						ETATT[i + 1].VTank = E3.VTank;
						ETATT[i + 1].Rank = E3.Rank;
						ETATT[i + 1].Gap = E3.Gap;
					}
				}

			}
			if ((ETATT[i + 1].Rank == S) && (ETATT[i + 1].VTank >= s0))
			{
				Not_success = false;
				BSUP_Prod = ETATT[i + 1].Wprod;
				cout << "Pipeline : Le cout de la solution BSUP pour la production est : " << ETATT[i + 1].Wprod << endl;
			}

		}
		i += 1;

	}//while (Not_Failure && Not_success)
	ofstream ArmonFlux_instances_excel_EXP("ArSolutionProgDyn_NS.csv", ifstream::app);
	if (BSUP_Prod == INFINI) //on n'a pas trouve de solution
	{
		//stopProg(" Heuristique::BSUP_Production_flexible : pas de solution (liste vide)");
		std::cout << "pas de solution (liste vide)" << endl;

		ArmonFlux_instances_excel_EXP << Nb_stations << ";" << TMAX << ";" << num_file << ";" << -1 << endl;

		std::string cheminbsup = "cout_BSUP_prod_pipeline.txt";
		ofstream cheminbsupFlux_instances(cheminbsup, ifstream::app);
		cheminbsupFlux_instances << -1 << endl;

	}
	else//on a trouve une solution 
	{
		ArmonFlux_instances_excel_EXP << Nb_stations << ";" << TMAX << ";" << num_file << ";" << BSUP_Prod << endl;
		std::string cheminbsup = "cout_BSUP_prod_pipeline.txt";
		ofstream cheminbsupFlux_instances(cheminbsup, ifstream::app);
		cheminbsupFlux_instances << BSUP_Prod << endl;
	}
	ArmonFlux_instances_excel_EXP.close();
}

Val_nb_State_pipeline Heuristique::Pipeline_T_Reconstruction_procedure(string num_file, double CpuT, bool Filtrage_exacte_pipe_,bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_, int BSUP)
{
#ifdef DESSINER_PIPELINE
	Programdyn P;
	SolutionSMEPC sol = P.sol;
	sol.periodeProd.resize(Nombre_de_periode, false);
	sol.periodeRecharge.resize(Nombre_de_periode, false);
	sol.stationAvantRecharge.resize(Nb_stations + 1, false);
	sol.quantiteRecharge.resize(Nb_stations + 1, 0);
	int ind_recg_dessin = 0;
#endif	
	//Reconstruct the whole solution
	T_PIPELINE[0] = 0;
	int i_aux = 0;
	for (int j = 0; j <= Nb_stations; j++)
	{
		if (SolutionPipelineRech[j].x == 0)
		{
			T_PIPELINE[j + 1] = T_PIPELINE[j] + d_i[j];
		}
		else
		{
#ifdef DESSINER_PIPELINE
			sol.stationAvantRecharge[j] = true;
			sol.quantiteRecharge[ind_recg_dessin] = SolutionPipelineRech[j].L;
			ind_recg_dessin++;
#endif
			//Compute the smallest i0 >= i-aux such that Delta_i0 = 1;
			int i0 = i_aux;
			while (i0 < Nombre_de_periode && SolutionPipelineProd[i0].delt == 0) i0++;
			T_PIPELINE[j + 1] = p * (i0 + 1) + d_0i[j + 1];
			i_aux = i0 + 1;

		}
	}

	int Prod_VAL = 0;
	for (int i = 0; i < Nombre_de_periode; i++)
	{
#ifdef DESSINER_PIPELINE
		if (SolutionPipelineProd[i].z == 1)sol.periodeProd[i] = true;
		if (SolutionPipelineProd[i].delt == 1)sol.periodeRecharge[i] = true;
#endif
		Prod_VAL += (Cf*SolutionPipelineProd[i].y + coutv[i] * SolutionPipelineProd[i].z);
	}

	VAL_PIPELINE = Prod_VAL + beta * (FINAL.Wprod - Prod_VAL + p + temps_restant_st_S__0); //cout de la solution heuristique au sens prog dyn global
	if (VAL_PIPELINE > BSUP&&Filtrage_exacte_pipe_==true&& PIPELINE_SANS_filtrage_BSUP_==false &&PIPELINE_SANS_filtrage_ == false&& Filtrage_Heuristique_Pipe_ == false ) VAL_PIPELINE = INFINI;//23/05/2022 si en ajoutant + p + temps_restant_st_S__0  au cout, on depasse la Bsup alors cette solution n'est pas valide
	if (VAL_PIPELINE > BSUP&&Filtrage_exacte_pipe_ == false && PIPELINE_SANS_filtrage_BSUP_ == false && PIPELINE_SANS_filtrage_ == false && Filtrage_Heuristique_Pipe_ == true) VAL_PIPELINE = INFINI;//23/05/2022 si en ajoutant + p + temps_restant_st_S__0  au cout, on depasse la Bsup alors cette solution n'est pas valide

#ifdef GENERER_INSTANCE_RESEAU_NEURONES_suite_suite_T_cost_A_cost_P
	int TTT, cost_A = 0, cost_P = 0;
	TTT = FINAL.Wprod - Prod_VAL;
	for (int i = 0; i < Nombre_de_periode; i++)
	{
		cost_A += Cf * SolutionPipelineProd[i].y;
		cost_P += coutv[i] * SolutionPipelineProd[i].z;
	}
	string chemiReseauN_s = "InstanceReseauNeurones_suite_suite/RN_instance_prod_suite_suite__" + num_file;
	chemiReseauN_s = chemiReseauN_s + ".txt";
	ofstream monwwFlux_instances_Reseau_Neuronne_ss(chemiReseauN_s, ifstream::app);
	monwwFlux_instances_Reseau_Neuronne_ss << TTT << endl;
	monwwFlux_instances_Reseau_Neuronne_ss << cost_A << endl;
	monwwFlux_instances_Reseau_Neuronne_ss << cost_P << endl;
	for (int s = 1; s < S; s++)
		monwwFlux_instances_Reseau_Neuronne_ss << Deltaa[s] << " ";
	monwwFlux_instances_Reseau_Neuronne_ss << endl;
#endif
	//cout << "Le cout (distance) avec attente :" << T_PIPELINE[Nb_stations + 1] << endl;
	cout << "le cout de production est :" << Prod_VAL << endl;
	//int val = FINAL.Wprod + beta * (p + temps_restant_st_S__0);
#ifdef VERIF_
	if (val != VAL_PIPELINE) stopProg("Heuristique::Pipeline_T_Reconstruction_procedure: !!!!!!!!!!!!!!Valeur du coût global DPS mal calculée");
#endif // VERIF_

	cout << "Le cout de cette solution est :" << VAL_PIPELINE << endl;
	cout << "La valeur de p est :" << p << endl;
	cout << " la distance temps_restant est :" << temps_restant_st_S__0 << endl;

	//J'ecris la solution dans le fichier excel ArSolutionProgDyn_Pipeline.csv (debut)
//cout << "Le cout de production avec les lambdas :" << FINAL.Wprod << endl;

	if (VAL_PIPELINE >= INFINI) //on n'a pas trouve de solution
	{
		VAL_PIPELINE = -1;
		//stopProg(" Heuristique::dyn_Production_flexible : pas de solution (liste vide)");
		std::cout << "pas de solution (liste vide)" << endl;
		ofstream ArmonFlux_instances_excel_EXP("ArSolutionProgDyn_Pipeline_" + num_file + ".csv");//, ifstream::app

		ArmonFlux_instances_excel_EXP << Nb_stations << ";" << TMAX << ";" << num_file << ";" << p << ";" << -1 << ";" << temps_restant_st_S__0 << ";" << Nombre_etats_maximal_prod << ";" << Nombre_etats_maximal_recharge << ";" << CpuT << endl;

		ArmonFlux_instances_excel_EXP.close();
	}
	else
	{

		//DEBUG : affichage du chemin optimal
		Etat_prod e_courant = FINAL;
		int periode = FINAL_TIME;
		cout << "Affichage de pi(3)";
		while (e_courant.Deltaant != -1)
		{
			//cout << periode <<" "<< e_courant.Deltaant << " " << e_courant.Gap << " " << e_courant.Rank << " " << e_courant.VTank << " " << e_courant.Wprod << " " << e_courant.Z << " " << e_courant.Zant << endl;
			periode = periode - 1;
			e_courant = PROD[periode][e_courant.Pred];
		}

		ofstream ArmonFlux_instances_excel_EXP("ArSolutionProgDyn_Pipeline_" + num_file + ".csv");//, ifstream::app

		ArmonFlux_instances_excel_EXP << Nb_stations << ";" << TMAX << ";" << num_file << ";" << p << ";" << VAL_PIPELINE << ";" << temps_restant_st_S__0 << ";" << Nombre_etats_maximal_prod << ";" << Nombre_etats_maximal_recharge << ";" << CpuT << endl;

		ArmonFlux_instances_excel_EXP.close();
	}

	cout << "valeur du pipeline = " << VAL_PIPELINE << endl;
	cout << "Nombre_etats_maximal_prod= " << Nombre_etats_maximal_prod << endl;


#ifdef DESSINER_PIPELINE


	//Costruction du fichier data_***.txt qui nous permettra de déssiner la solution de chaque instance
	std::string chemin__1{ ".txt" };
	std::string chemin1 = "data_" + num_file + chemin__1;
	ofstream Dessin_instances(chemin1);
	Dessin_instances << Nombre_de_periode << endl;
	Dessin_instances << p << endl;
	Dessin_instances << Nb_stations << endl;
	vector <int> Hydro_depot_arrive_quitte;
	vector <int> Date_depot_arrive_quitte;
	Hydro_depot_arrive_quitte.resize(Nb_stations);
	Date_depot_arrive_quitte.resize(Nb_stations);
	//qte H2 dans citerne en fin periode i = 0..N(pour i = 0 : qte initiale) 
	Dessin_instances << s0 << " ";
	int Tank = s0;
	int k = 0; //Numéro de la recharge à effectuer
	for (int i = 0; i < Nombre_de_periode; i++)
	{
		if (sol.periodeProd[i] == false && sol.periodeRecharge[i] == false)
			Dessin_instances << Tank << " ";
		if (sol.periodeProd[i] == false && sol.periodeRecharge[i] == true)
		{
			Tank = Tank - sol.quantiteRecharge[k];
			Dessin_instances << Tank << " ";
			k++;

		}
		if (sol.periodeProd[i] == true && sol.periodeRecharge[i] == false)
		{
			Tank = Tank + rendement[i];
			Dessin_instances << Tank << " ";
		}

	}

	Dessin_instances << endl;

	//dates auxquelles le vehicule arrive aux stations : 0..M + 1 (1ere valeur = 0 = depart, derniere = retour au depot)
	int jj = 0, Q = 0;
	Dessin_instances << 0 << " ";
	int T = 0;
	for (int j = 0; j < Nb_stations + 1; j++)
	{

		if (sol.stationAvantRecharge[j] == false)
		{
			T = T + d_i[j];
			Dessin_instances << T << " ";
		}
		else
		{
			if (sol.stationAvantRecharge[j] == true)
			{
				Date_depot_arrive_quitte[jj] = T + d_i0[j];
				//On cherche l'indice de période ind_periode qui correspond à la (Q+1) ième recharge c'est-à-dire où tes.sol.periodeRecharge[ind_periode]==true
				int ind_periode = 0;
				int per_rech = 0;
				while (ind_periode < Nombre_de_periode && per_rech < Q + 1)
				{
					if (sol.periodeRecharge[ind_periode] == true)
						per_rech++;
					ind_periode++;
				}
				Q++;
				Date_depot_arrive_quitte[jj + 1] = ind_periode * p;
				jj += 2;
				T = ind_periode * p + d_0i[j + 1];
				Dessin_instances << T << " ";

			}

		}
	}

	Dessin_instances << endl;

	//dates auxquelles le vehicule arrive / quitte l'usine (ordre chronologique)
	for (int j = 0; j < jj; j++)
	{
		Dessin_instances << Date_depot_arrive_quitte[j] << " ";
	}
	Dessin_instances << endl;

	//H2 dans le reservoir du vehicule quand il arrive aux stations 0..M + 1 (0->qte initiale)
	Dessin_instances << v0 << " ";
	int Reservoir = v0;
	k = 0;
	jj = 0;
	for (int j = 0; j < Nb_stations + 1; j++)
	{
		if (sol.stationAvantRecharge[j] == false)
		{
			Reservoir = Reservoir - E_i[j];
			Dessin_instances << Reservoir << " ";
		}
		else
		{
			if (sol.stationAvantRecharge[j] == true)
			{
				Hydro_depot_arrive_quitte[jj] = Reservoir - E_i0[j];
				Hydro_depot_arrive_quitte[jj + 1] = Reservoir - E_i0[j] + sol.quantiteRecharge[k];
				Reservoir = Reservoir - E_i0[j] + sol.quantiteRecharge[k] - E_0i[j + 1];
				jj += 2;
				k++;
				Dessin_instances << Reservoir << " ";
			}
		}

	}
	Dessin_instances << endl;
	//H2 dans le reservoir du vehicule quand il arrive / repart de l'usine
	for (int j = 0; j < jj; j++)
	{
		Dessin_instances << Hydro_depot_arrive_quitte[j] << " ";
	}
	Dessin_instances << endl;
	for (int i = 0; i < Nombre_de_periode; i++)
		Dessin_instances << rendement[i] << " ";
	Dessin_instances << endl;
	for (int i = 0; i < Nombre_de_periode; i++)
		Dessin_instances << coutv[i] << " ";
	Dessin_instances << endl;
	for (int j = 0; j < Nb_stations + 1; j++)
		Dessin_instances << d_i[j] << " ";
	Dessin_instances << endl;
	for (int j = 0; j < Nb_stations + 1; j++)
		Dessin_instances << E_i[j] << " ";
	Dessin_instances << endl;
	for (int j = 0; j < Nb_stations + 1; j++)
		Dessin_instances << d_i0[j] << " ";
	Dessin_instances << endl;
	for (int j = 0; j < Nb_stations + 1; j++)
		Dessin_instances << E_i0[j] << " ";
	Dessin_instances << endl;

	string chemin__t = "python main.py " + num_file;
	const char *Cmd_python = chemin__t.data();
	system(Cmd_python);

#endif

#ifdef GENERER_INSTANCE_RESEAU_NEURONES
	vector<int> recharge;
	recharge.resize(Nombre_de_periode, 0);
	int temp_s = 1;
	string chemiReseauN = "InstanceReseauNeurones/RN_instance_prod__" + num_file;
	chemiReseauN = chemiReseauN + ".txt";
	ofstream monwwFlux_instances_Reseau_Neuronne(chemiReseauN, ifstream::app);

	//ajouter cout de prod et date derniere recharge
	monwwFlux_instances_Reseau_Neuronne << Prod_VAL << endl;
	monwwFlux_instances_Reseau_Neuronne << FINAL.Wprod - Prod_VAL << endl;


	//recharges 
	while (u[temp_s] > 0 && temp_s <= Nb_stations + 1)
	{
		for (int k = mm[temp_s]; k < MM[temp_s]; k++)
		{
			recharge[k] += u[temp_s];
		}
		temp_s++;
	}
	for (int i = 0; i < Nombre_de_periode; i++)
		monwwFlux_instances_Reseau_Neuronne << recharge[i] << " ";
	monwwFlux_instances_Reseau_Neuronne << endl;
	monwwFlux_instances_Reseau_Neuronne.close();
#endif
#ifdef		GENERER_INSTANCE_RESEAU_NEURONES_suite_y_m_M

	string chemiReseauN_ss = "InstanceReseauNeurones_suite/RN_instance_prod_suite__" + num_file;
	chemiReseauN_ss = chemiReseauN_ss + ".txt";

	ofstream monwwFlux_instances_Reseau_Neuronne_s(chemiReseauN_ss, ifstream::app);
	monwwFlux_instances_Reseau_Neuronne_s << S << endl;

	for (int s = 1; s <= S; s++)
		monwwFlux_instances_Reseau_Neuronne_s << mm[s] << " ";
	monwwFlux_instances_Reseau_Neuronne_s << endl;

	for (int s = 1; s <= S; s++)
		monwwFlux_instances_Reseau_Neuronne_s << MM[s] << " ";
	monwwFlux_instances_Reseau_Neuronne_s << endl;

	for (int s = 1; s <= S; s++)
		monwwFlux_instances_Reseau_Neuronne_s << muS[s] << " ";
	monwwFlux_instances_Reseau_Neuronne_s.close();
#endif
	Val_nb_State_pipeline Sol_;
	Sol_.val = VAL_PIPELINE;
	Sol_.Nombre_etats_maximal_recharge = Nombre_etats_maximal_recharge;
	Sol_.Nombre_etats_maximal_prod = Nombre_etats_maximal_prod;
	Sol_.CPU = CpuT;
	return Sol_;

}

bool Heuristique::verifEtatValide_PIPELINE(Etat_prod &ef)
{
	bool ok = true;


	if (ef.Z != 0 && ef.Z != 1)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Z != 0 && ef.Z != 1" << endl;
	}
	if (ef.VTank < 0 || ef.VTank > smax)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINEef.VTank < 0 || ef.VTank > smax" << endl;
	}

	if (ef.Rank < 0 || ef.Rank > S + 1)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Rank < 0 || ef.Rank > S+1" << endl;
	}
	if (ef.Gap < 0 || ef.Gap > TMAX)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Gap < 0 || ef.Gap > TMAX" << endl;
	}
	if (ef.Wprod < 0)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Wprod < 0" << endl;
	}

	if (ef.Zant != 0 && ef.Zant != 1)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Zant != 0 && ef.Zant != 1" << endl;
	}
	if (ef.Deltaant != 0 && ef.Deltaant != 1)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE ef.Deltaant != 0 && ef.Deltaant != 1" << endl;
	}

	return ok;
}

bool Heuristique::verifEtatValide_PIPELINE_RECHARGE(Etat_Recharg & ef)
{

	bool ok = true;

	if (ef.V < 0 || ef.V > vmax)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE_RECHARGE ef.V < 0 || ef.V > vmax" << endl;
	}

	if (ef.T < 0 || ef.T > TMAX)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE_RECHARGE ef.T < 0 || ef.T > TMAX" << endl;
	}

	if (ef.xx != 0 && ef.xx != 1)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE_RECHARGE ef.xx != 0 && ef.xx != 1" << endl;
	}

	if (ef.perf < 0)
	{
		ok = false;
		cout << "Heuristique::verifEtatValide_PIPELINE_RECHARGE ef.perf < 0" << endl;
	}


	return ok;
}

#ifdef	OPTIMISATION_TOUR
string Heuristique::REMOVAL_STATS(string num_file, float BETAA)
{
	//!!!!!!!!!Attention cet algo n est pas encore correcte (cad conforme au papier d'Al)
	string J; //Stations à retirer car elles engendrent un très grand détour (supérieur à la moyenne au sens temps et energie des détours) (beta*Energie+alpha*temps)
	Heuristique test;
	string fichier_production = "instances_avec_p_opt_tour/instance_Prod__";
	string fichier_instance = "instances_avec_p_opt_tour/instance__";
	test.read_instance_in_file(fichier_instance, num_file, fichier_production);
	vector<int> tab_gain_by_pt;
	std::vector<int>::iterator it_tab_gain_by_pt;
	tab_gain_by_pt.resize(test.Nb_stations + 1, -1);

	test.Tri_bulle();
	test.Estimation_energie_temps();
	test.dyn_Recharge_temps();
	test.dyn_Recharge_energie();
	test.Cout_min_prod();

	//exécuter DPS_véhicule pour récupérer la statégie de recharge réduite

	float alphaa = BETAA; //energie
	int betaa = 1;//temps
	test.dyn_Recharge(alphaa, betaa);
	test.intervalles_recharge();
	test.extraction_tour(num_file, alphaa, betaa);
	vector<int> La_tournee;
	std::vector<int>::iterator it_La_tournee;
	std::list<int>::iterator it_tr;
	La_tournee.resize(size(test.tourne), -1);
	tab_gain_by_pt.resize(test.Nb_stations + 1, -1);
	//On transforme test.tourne pour pouvoir l'utiliser lors du calcul des gains de chaque station
	//La_tournee[i] est la tournée de numfile avec les numéro de station de test.liststations
	int i = 0;
	for (it_tr = test.tourne.begin(); it_tr != test.tourne.end(); ++it_tr)
	{
		if (*it_tr == -1)
		{
			La_tournee[i] = test.Nb_stations + 2;
			i++;
		}
		else
		{
			if (*it_tr == test.Nb_stations + 1)
			{
				La_tournee[i] = 0;
				i++;
			}
			else
			{
				La_tournee[i] = *it_tr;
				i++;
			}

		}

	}
	int som_gain = 0;
	for (int i = 1; i <= test.Nb_stations; i++)
	{
		int k = i;
		while (i != La_tournee[k])k++;
		//Calcul des gains tab_gain_by_pt[i] de chaque station i. On ne compte pas la station 0 car on ne peut pas l'enlever.
		tab_gain_by_pt[i] = betaa * (test.distanceEuclidienne(La_tournee[k - 1], La_tournee[k]) + test.distanceEuclidienne(La_tournee[k], La_tournee[k + 1]) - test.distanceEuclidienne(La_tournee[k - 1], La_tournee[k + 1])) + alphaa * (test.distanceManhattan(La_tournee[k - 1], La_tournee[k]) + test.distanceManhattan(La_tournee[k], La_tournee[k + 1]) - test.distanceManhattan(La_tournee[k - 1], La_tournee[k + 1]));
		som_gain += tab_gain_by_pt[i];
	}
	int moy_gain = som_gain / test.Nb_stations;
	//On retire toutes les stations dont le gain est supérieur à la moyenne des gains ???????????????????
	for (int i = 1; i <= test.Nb_stations; i++)
	{
		if (tab_gain_by_pt[i] > moy_gain)
			J += to_string(i) + "_";
	}
	//if (tab_gain_by_pt[test.Nb_stations] > moy_gain)
	//	J += to_string(test.Nb_stations);


	//REMOVAL_STATS avec la fonction random
	/*
	string J;
	Heuristique test;
	string fichier_production = "instances_avec_p_opt_tour/instance_Prod__";
	string fichier_instance = "instances_avec_p_opt_tour/instance__";
	test.read_instance_in_file(fichier_instance, num_file, fichier_production);
	//génére les nombres entre a et b : a + (int)((float)rand() * (b - a + 1) / (RAND_MAX - 1));
	//génération d'un nombre entre 1 et Nbstations/2
	int Nb_point_enlev = 1 + (int)((float)rand() * ((test.Nb_stations/2) - 1 + 1) / (RAND_MAX - 1));
	vector<int> tab_pt;
	std::vector<int>::iterator it_tab_pt;
	tab_pt.resize(Nb_point_enlev, -1);
	for (int i = 0; i < Nb_point_enlev; i++)
	{
		int pt_a_enlev= 1 + (int)((float)rand() * (test.Nb_stations - 1 + 1) / (RAND_MAX - 1));
		it_tab_pt = std::find(tab_pt.begin(), tab_pt.end(), pt_a_enlev);
		while (it_tab_pt != tab_pt.end())
		{
				pt_a_enlev = 1 + (int)((float)rand() * (test.Nb_stations - 1 + 1) / (RAND_MAX - 1));
				it_tab_pt = std::find(tab_pt.begin(), tab_pt.end(), pt_a_enlev);
		}
		tab_pt[i]=pt_a_enlev;


		J += to_string(pt_a_enlev) +  "_";
	}*/

	return J;

}
void  Heuristique::INSERT_STATS(string J, string num_file, float BETAA)
{
	//On récupère les numéros de stations à enlever du tour
	list <int> J_int;

	std::string s = J;
	std::string delimiter = "_";
	size_t pos = 0;
	std::string sousChaine;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		sousChaine = s.substr(0, pos);
		J_int.push_back(stoi(sousChaine));
		s.erase(0, pos + delimiter.length());
	}
	//J contient les points à ajouter dans le nouveau tour
	Heuristique test;
	string fichier_production = "instances_avec_p_opt_tour/instance_Prod__";
	string fichier_instance = "instances_avec_p_opt_tour/instance__";
	test.read_instance_in_file(fichier_instance, num_file, fichier_production);
	string const nomFichier("instances_avec_p_opt_tour/instance__" + num_file + "0.txt");
	ofstream monFlux(nomFichier.c_str());
	if (monFlux)   //On teste si tout est OK
	{

		monFlux << test.Nb_stations - size(J_int) << endl;//nombre de stations
		monFlux << test.TMAX << endl;//TMAX
		monFlux << test.v0 << endl;
		monFlux << test.vmax << endl;
		monFlux << test.p << endl;//delta devient p la taille d'une période
		monFlux << test.alpha << endl;//alpha
		monFlux << test.beta << endl;//beta
		monFlux << test.Cf << endl;//cout fixe
		monFlux << test.s0 << endl;
		monFlux << test.smax << endl;
		monFlux << " " << endl;
		monFlux << "stations" << endl;
		for (int i = 0; i <= test.Nb_stations; i++)
		{
			if (find(J_int.begin(), J_int.end(), i) == J_int.end())
			{
				monFlux << test.list_stations[0][i] << endl;
				monFlux << test.list_stations[1][i] << endl;
			}

		}
		monFlux << "FIN" << endl;
	}
	else
	{
		printf("Le fichier n'existe pas");
	}

	//évaluation de la nouvelle instance losrqu'on a retiré les stations
	//Heuristique test;
	//string fichier_production = "instances_avec_p_opt_tour/instance_Prod__";
	//string fichier_instance = "instances_avec_p_opt_tour/instance__";
	test.read_instance_in_file(fichier_instance, num_file + "0", fichier_production);
	test.Tri_bulle();
	test.Estimation_energie_temps();
	test.dyn_Recharge_temps();
	test.dyn_Recharge_energie();
	test.Cout_min_prod();

	//exécuter DPS_véhicule pour récupérer la statégie de recharge réduite

	float alphaa = BETAA; //energie
	int betaa = 1;//temps
	test.dyn_Recharge(alphaa, betaa);
	test.intervalles_recharge();
	test.extraction_tour(num_file + "0", alphaa, betaa);

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Touver la meilleure position pour insérer chaque station  de J_int, 
	//sachant que test contient la solution pipeline du nouveau (sans les stations J_int) 



}
float Heuristique::EVALUATION_TOUR_dps_vehicule(string num_file, float BETAA)
{
	//initialiser le tour

	Heuristique test;
	string fichier_production = "instances_avec_p_opt_tour/instance_Prod__";
	string fichier_instance = "instances_avec_p_opt_tour/instance__";
	test.read_instance_in_file(fichier_instance, num_file, fichier_production);
	test.Tri_bulle();
	test.Estimation_energie_temps();
	test.dyn_Recharge_temps();
	test.dyn_Recharge_energie();
	test.Cout_min_prod();

	//exécuter DPS_véhicule pour récupérer la statégie de recharge réduite

	float alphaa = BETAA; //energie
	int betaa = 1;//temps
	test.dyn_Recharge(alphaa, betaa);
	test.intervalles_recharge();
	test.extraction_tour(num_file, alphaa, betaa);

	printf("EVALUATION_TOUR_dps_vehicule");
	return test.cout_final_heuris_temps_energie;

}

float Heuristique::EVALUATION_TOUR_learning()
{
	float VAL_PROD_RN = 0;
	return VAL_PROD_RN;
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

}

void Heuristique::OPT_TOUR(int seuil, string num_file, float BETAA)
{
	bool not_stop = true;
	//initialiser le tour et exécuter DPS_véhicule pour récupérer la statégie de recharge réduite
#ifdef OPTIMISATION_TOUR_dps_vehicule
	float W = EVALUATION_TOUR_dps_vehicule(num_file, BETAA);//exécuter l'algorithme d'évaluation du tour et mettre le cout dans W
#endif
#ifdef OPTIMISATION_TOUR_learning
	float W = EVALUATION_TOUR_learning();//exécuter l'algorithme d'évaluation du tour et mettre le cout dans W
#endif
	float W_Aux;
	while (not_stop)
	{
		int counter = 0;
		bool not_stop1 = true;
		while (counter <= seuil & not_stop1)
		{
			//générer un sous-ensemble de stations J
			string J = REMOVAL_STATS(num_file, BETAA);
			//insérer les stations J dans le tour d'une autre facon
			INSERT_STATS(J, num_file, BETAA);
			//exécuter DPS_véhicule pour récupérer la statégie de recharge réduite
#ifdef OPTIMISATION_TOUR_dps_vehicule
			W_Aux = EVALUATION_TOUR_dps_vehicule(num_file + "0", BETAA);//exécuter l'algorithme d'évaluation du tour et mettre le cout dans W_Aux
#endif
#ifdef OPTIMISATION_TOUR_learning
			W_Aux = EVALUATION_TOUR_learning();//exécuter l'algorithme d'évaluation du tour et mettre le cout dans W
#endif
			if (W_Aux < W)
			{
				//l'ancien tour devient le nouveau tour (l'ancien fichier contenant le tour optimal est remplacé)
				string fichier_instance = "instances_avec_p_opt_tour/instance__";
				std::string chemin;
				std::string chemin_{ ".txt" };
				chemin = fichier_instance + num_file + chemin_;
				ofstream monreadFlux_instances(chemin);
				if (!monreadFlux_instances)
					cout << "Heuristique::read : erreur : fichier non ouvert" << endl;

				std::string chemin2;
				std::string chemin__{ "0.txt" };
				chemin2 = fichier_instance + num_file + chemin__;
				ifstream monreadFlux_instances2(chemin2);
				if (!monreadFlux_instances2)
					cout << "Heuristique::read : erreur : fichier non ouvert" << endl;

				if (monreadFlux_instances && monreadFlux_instances2)  //On teste si tout est OK
				{
					string ligne;
					while (getline(monreadFlux_instances2, ligne))
					{
						monreadFlux_instances << ligne << endl;

					}

				}
				//l'ancien recharge reduite devient la nouvelle recharge reduite
				W = W_Aux;
				not_stop1 = false;
			}
			else
			{
				counter++;
				if (counter > seuil)
					not_stop = false;
			}
		}
	}


}


#endif