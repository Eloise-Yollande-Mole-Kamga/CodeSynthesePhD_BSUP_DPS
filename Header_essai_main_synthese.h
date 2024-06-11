#pragma once
#pragma once
#ifndef __HeaderdynGlobal_H__
#define __HeaderdynGlobal_H__

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <tuple>
#include <iterator>
#include <numeric>
#include <list>
const int ETAT_MAX = 400000;
//Programme dynamique global du SMEPC
#ifndef DBL_EPSILON
const double DBL_EPSILON = 0.000001;
#endif

//==================================================Hearder progdyn
#define PROGRAMME_DYNAMIQUE //AVEC Filtrage RN
//#define Filtrage_exacte //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
//#define PROGRAMME_DYNAMIQUE_SANS_filtrage //On active ceci si on veut enlever le filtrage
//#define PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP //On active ceci si on veut enlever le filtrage bSUP 
//#define Filtrage_Heuristique //On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
//const int K_FILTRAGE_HEURIS = 7;

//---------------------------------------Choisir une BSUP
//#define BSUP_Greedy_20
//#define BSUP_Greedy_50// BSUP NS 50
//#define BSUP_Helene//BSUP PL Helene//#define BSUP_Greedy_10 //RN
//#define BSUP_Greedy_100
//#define BSUP_Pipeline
//#define BSUP_PL_1_seconde
//#define BSUP_Greedy_1

//#define PARTIE_COMMUNE_AVANT_OPT_TOUR //Si on veut exécuter autre chose que l'optimisation du tour il faut activer ceci//....//#
//#define NEW_INSTANCE //Ceci permet de traiter les instances du dossier instances_avec_p/


//=========================Header Pipeline
//#define VERIF_

//#define PIPELINE //RN
//#define PIPELINE_dyn_Production_flexible //RN
//#define DESSINER_PIPELINE

//#define Filtrage_exacte_pipe //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
//#define PIPELINE_SANS_filtrage_BSUP
//#define PIPELINE_SANS_filtrage //On active ceci si on veut enlever le filtrage
//#define Filtrage_Heuristique_Pipe //On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 09/05/2022
//const int K_FILTRAGE_HEURIS_Pipe = 3;


//#define BSUP_Greedy_50// BSUP NS 50
//#define BSUP_Helene//BSUP PL Helene
//#define BSUP_prod_Helene
//#define BSUP_Greedy_20
//#define BSUP_Greedy_50
//#define BSUP_Greedy_100
//#define PARTIE_COMMUNE_AVANT_OPT_TOUR //Si on veut exécuter autre chose que l'optimisation du tour il faut activer ceci//....//
//#define NEW_INSTANCE //Ceci permet de traiter les instances du dossier instances_avec_p/

//#define NEW_INSTANCE //Ceci permet de traiter les instances du dossier instances_avec_p/
//const int NB_ETAT_RECHARG = 50000;
//const int NB_ETAT_prod = 50000; // nombre maximum d'états crée par le prog prod
///const int INFINI = 1000000;
///const int COUT_MAXIMAL_NS = 10000;
//const int COUT_BSUP_INST = 16000; // modif 9/03/2022 => on passe a 16000 pour generer les instances pour les reseaux de neurones
//const int BSUP_MAX = 10000;


//=======================HearderNS
//#define NS_PROGRAMME_DYNAMIQUE 
//#define Filtrage_exacte

//#define BSUP_Greedy_50// BSUP NS 50
//#define BSUP_Helene//BSUP PL Helene
//#define PARTIE_COMMUNE_AVANT_OPT_TOUR //Si on veut exécuter autre chose que l'optimisation du tour il faut activer ceci//....//#
//#define NEW_INSTANCE //Ceci permet de traiter les instances du dossier instances_avec_p


const int NB_ETAT_RECHARG = 50000;
const int NB_ETAT_prod = 50000; // nombre maximum d'états crée par le prog prod
const int INFINI = 1000000;
const int COUT_MAXIMAL_NS = 10000;
const int COUT_BSUP_INST = 31;//16000; // modif 9/03/2022 => on passe a 16000 pour generer les instances pour les reseaux de neurones
const int BSUP_MAX = 10000;
const int TEMPS_LIMIT = 3600;//3600; //Temps limite en seconde
using namespace std;
//const int p = 2; //=> avec les instances transformées instance_Prod_avecp_ la valeur de p est 2
//const int p = 4; //=> donnée de l'instance A METTRE DANS LE FICHIER D INSTANCE
//avec l'instance 100 le p vaut 4

inline void stopProg(string msg)
{
	char stop;
	cout << msg << endl;
	cin >> stop;
}


//==============================================================================================================================================
// Ce projet résout le problème le SMEPC : synchronized management of energy production and consumption
// tel que decrit dans l'article soumis a RAIRO en février 2020 

// dans cette branche du projet on prend en compte une durée de periode p eventuellement > 1

//  le retour au depot est realise avec une citerne et un reservoir au moins egales a l'etat initial


// Les fonctions principales (VEH_maitre_PROD_esclave_sans_NS, VEH_maitre_PROD_esclave et BSUP_VEH_maitre_PROD_esclave) s'appuie sur un programme dynamique où
// les INSTANTS au sens programmation dynamique sont (t,i) : t = période ([t,t+1]) et i = station

// les ETATS, associes a un instant (t,i) sont (E, delta, cit, CH_h2, pere) : 
//   - E = 1 si usine active sur [t-1,t] ; 
//   - delta in [0, TMAX] = 
//        - si delta > t : le véhicule va a la station i, il arrivera à la date delta
//        - si delta < t : le véhicule est entre i et l'usine (eventuellement en train d'attendre)
//        - si delta = t : le véhicule est en i, il doit choisir s'il continue vers i+1 ou va a l'usine
//   - cit = quantite dans la citerne a la fin de la periode [t-1,t]
//   - CH_h2 = quantite dans le reservoir quand le vehicule arrive a la station i
//   - pere est decrit par des entiers qui permettent de retrouver l'etat pere dans le tableaux des etats

// les DECISIONS (a,b,c), pour un instant (t,i) et un etat e = (E, delta, cit, CH_h2, pere) sont :
//    - a = 1 si usine produit pendant periode [t,t+1[
//    - b = utile uniquement si e.delta = t : auquel cas si b = 1 alors on continue de i vers i+1 sans faire le plein
//    - c = 1 : le véhiucle est a l'usine et va recharger
// les decisions sont prises a la fin de la periode [t-1,t[


//========================================================================
// classe Heuristique : lit et stocke une instance du SMEPC
//La classe heuristique contient aussi des algorithmes qui resolvent le SMEPC de facon heuristique à l'aide d'une procedure ne pipeline

// dans cette branche du projet on prend en compte une durée de periode p eventuellement > 1

// le retour au depot est realise avec une citerne et un reservoir au moins egales a l'etat initial

//On a deux parties : 

//La partie traitant le PROBLEME VEHICULE
// La fonction principale (dyn_Recharge) s'appuie sur un programme dynamique où

// l'INSTANT au sens programmation dynamique est (i) : i = station (0...Nombre de stations+1 )

// les ETATS, associes a un instant (i) sont (V,T, pere) : 
//		- V = energie dans le vehicule en i
//		- T = date de passage du vehicule en i
//pere est decrit par un entier qui permet de retrouver l'etat pere dans le tableaux des etats

//les DECISIONS x pour l'instant i et un etat (V,T, pere) sont :
//		- x = 0 le vehicule se deplace de la sation i a la station i+1
//		- x = 1 le vehicule se recharger entre la station i et la station i+1

//La partie traitant le PROBLEME PRODUCTION

// La fonction principale (dyn_Production_flexible) s'appuie sur un programme dynamique où

// l'INSTANT au sens programmation dynamique est (t) : t = station (0 ...Nombre de p periode)

// les ETATS, associes a un instant (t) sont (Z, VTank, Rank, Gap, pere) : 

//		- Z_t = Etat de la machine de production d'hydrogene au debut de la periode t
//		- V^Tank = stock d'hydrogene dans la citerne au debut de la periode t
//		- Rang (\in 1..S) = l'opération de ravitaillement en carburant portant 
		  //le numéro Rang a ete effectuee et que nous attendons pour effectuer 
		 //la prochaine operation de ravitaillement en carburant portant le numero Rang + 1.  
//		- Gap = l'ecart entre t et la periode a laquelle la Rang ieme recharge  a ete effectuee. 
//		- pere est decrit par un entier qui permet de retrouver l'etat pere dans le tableaux des etats

//les DECISIONS (z,delta) pour l'instant t et un etat (Z, VTank, Rank, Gap, pere) sont :
//		- z = 1 si l'usine produit de l'hydrogene a la periode [t,t+1]
//		- delta = 1 si le vehicule se recharge en hydrogene a la periode [t,t+1] 
//========================================================================

//Solution du pipeline
struct Val_nb_State_pipeline
{
	int val;
	int Nombre_etats_maximal_recharge;
	int Nombre_etats_maximal_prod;
	float CPU;
};

//Solution des prog dyn
struct Val_nb_State_progdyn
{
	int val;
	int Nombre_etats_maximal;
	float CPU;
};

//Solution des Bsup
struct Val_Cpu_Bsup
{
	int val;
	float CPU;
};

class Heuristique
{

public:
	//Modifications des instances pour tenir compte de p!=1

	int Nombre_de_periode;
	//fin
	//float BSUP[COUT_BSUP_INST];
	//float BSUP_prod_pipeline[COUT_BSUP_INST];
	int Nb_stations = 0;
	int TMAX = 0;
	int QMAX = 0;
	int v0 = 0;
	int vmax = 0;
	int p = 0; //J'ai remplacé delta par p
	int alpha = 0;

	int Cf = 0;
	int s0 = 0;
	int smax = 0;
	//	int  lamda = 0;
	//	int coef_obj = 0; //cet élément n'est pas utilisé ici, il nous aide seulement pour faire la lecture du fichier pour la parti PLNE

	int PMAX = 0; //quanté max qui peut être produite
	vector<int> rendement; //quantité produite à t
private:
	vector<int> distmin; //distance tour i+1,i+2....0
	vector<int> q_; // q_[t] = quantité maximum de production possible de la periode 0 à la periode t (compris)
public:
	vector<int> coutv;//cout de production à t
private:
	//int VAL[1][2][1]; //cout minimal de production d'une quantité P

	int Q; //nombre de recharge
	vector<int> u; //quantité d'hydrogène chargée à la recharge q
	vector<int> u_; //quantité d'hydrogène chargée aux recharges 1,2,... q
	vector<float> A; //borne inférieure pour la date de la recharge q
	vector<float> B; //borne supérieure pour la date de la recharge q
public:

	vector<vector<float> > list_stations; // coordonnées des stations
	vector<int> d_i; //d_i,i+1     /d
	vector<int> d_i0; //d_i,0      /d0
	vector<int> d_0i; //d_0,i      /d0*
	vector<int> d_0; //distance supplementaire si détour = d(i,0) + d(0,i+1) - d(i,i+1) /d*
	vector<int> d_; // distance 0 à i-1
	vector<int> E_i; //E_i,i+1
	vector<int> E_0i; //E_0,i
	vector<int> d__; // distance i à 0
	vector<int> E_i0; //E_i,0
	vector<int> E_0; //le détour  E(i0) + E(0i+1)
	vector<int> E_; // E_[i] = énergie minimale (sans recharge) pour aller de  0 à i-1
	vector<int> E__; // énergie i à 0
private:
	vector<int> ORDOd; //liste des stations ordonnées par détour (distance) croissant
	vector<int> ORDOE; //liste des stations ordonnées par détour (énergie) croissant
public:
	double moy_cout;//moyene des couts totaux //new
	int Cout_temps;
	int beta;// = 0
	vector<int> q__; //q__[t] = quantité maximum de production possible de la periode t (compris) à la dernere periode
	vector<int> EQ; // énergie nécessaire pour aller du dépôt en i (en comptant les detours minimaux) RQ : en prend en compte qu'on doit rentrer au depot final avec v0
	vector<int> QQ; //nombre de recharge à faire pour arriver en i
	vector<int> TINF; //temps minimal qu'il faut pour arriver en i (en comptant les detours minimaux) RQ : en prend en compte qu'on doit rentrer au depot final avec v0
	vector<vector<vector<int> > > VAL; //cout minimal de production d'une quantité P
	int cout_final_heuris_temps;
	int cout_prod_flexible; //cout de production sans lambda
	int cout_prod_flexible_with_lambda; //cout de production avec lambda
	float cout_final_heuris_temps_energie;
private:
	vector<int> x; //vecteur recharge x[i]=1 le véhicule ira se charger entre la station i et la station i+1


	vector<int> REC; //indice d'accès au premier élément de la liste REC_
	struct Etat_Recharg //Etat
	{
		//pour un etat stocke dans REC[i] : 
		int V; //énergie dans le véhicule en i
		int T; // date de passage en i
		float perf; //coût en i
		int succ = -1; //liste chainee (non utilisee)
		int jant = -1; //indice du pere dans REC[i+1]
		int xx = -1; //decision : xx = 0 => on va de la station suiv, xx = 1 => on va a l'usine
		int tr = -1;//=0 si etat actif, -1 sinon
	};

	//Pour representer une solution du pipeline nous creons deux vecteurs de structure :
	//l'un de la strategie de production dont la taille est 0...Nombre_de_periode-1 :
	struct SolutionPipelineProduction
	{
		int z = 0;  //Vecteur de production 
		int y = 0; //Vecteur d'activation de la production
		int delt = 0; //Vecteur de recharge effective du véhicule à la micro-usine
	};
	vector <SolutionPipelineProduction> SolutionPipelineProd;
	//et l'autre constitue de la strategie de recharge dont la taille est 0...Nb_stations+2 :
	struct SolutionPipelineRecharge
	{
		int x = 0; //Vecteur de recharge du véhicule
		int L = 0; //Vecteur de la quantite rechargee 
	};
	vector <SolutionPipelineRecharge> SolutionPipelineRech;
	//Le vecteur temps
	vector <int> T_PIPELINE;
	//Le coût total de la solution pipeline
	int VAL_PIPELINE;

	vector<vector<Etat_Recharg> >  REC_; //liste des états pour le problème de recharges
	Etat_Recharg ETAT[3];

	vector<int> N_PROD; //N_PROD[i] = Nombre d'états de la liste d'états PROD[ i, ]
	struct Etat_prod //Etat
	{
		//On fait la correspondance entre ces variables et les variables de l'article pipeline
		int Z; // Z_i = Etat de la machine de production d'hydrogène au debut de la période i
		int VTank; // V^Tank = stock d'hydrogène dans la citerne
		int Rank; //Rang \in 1..S signifie que l'opération de ravitaillement en carburant portant 
				  //le numéro Rang a été effectuée et que nous attendons pour effectuer 
				 //la prochaine opération de ravitaillement en carburant portant le numéro Rang + 1.  
		int Gap;//Gap signifie la différence entre i et la période à laquelle la transaction de ravitaillement
				//du Rang a été effectuée. 
		//int AAA; //Date au plus tôt courante pour la recharge q
		int Wprod; //WProd est la valeur actuelle au sens de Bellman, cout de production  à cet état
		int Zant; int Deltaant; //(Zant, Deltaant) est la décision prise au début de la période i - 1;
		int Pred;  //Pred est l'indice q* tel que l'état actuel E dérive de PROD[i - 1, q*].

	};
	struct prod_dec
	{
		int y = -1;//décision de switch de la machine de production d'hydrogène=-1
		int R = -1; //décisiosn de recharge du véhicule=-1

	};


	vector<vector<Etat_prod> > PROD; //liste des états pour le problème de production flexible
	vector<Etat_prod> ETATT; //liste des états pour le Bsup prod flexible
	//Etat_prod ETAT_PROD[4];
	//int Ind[4];
	//int Ind2[4];
	//int W[4];
	vector<int> IND;
	int BSUP_prod;
	//int BSUP_prod2[TMAX];
	int t_cour = -1;//= -1
	int j_cour = -1;//= -1
	//std::list <Etat_Recharg> list_recharge;

	//pipeline
	vector<int> St; //St[s] is the station j such that the sth refueling transaction is performed between j and j+1;
	vector<float> Deltaa; //s the minimal delay between the date when the sth refueling transaction may start and the date when the next one (s+1) may start (or the end of the trip if s = S)
	vector<int> BS; //BS[s] = nombre de période séparant date au plus tot de la recharge s et date au plus tot de la recharge s+1 
	vector<int> muS; //muS[s]= Quantite d'hydrogene recharger par le vehicule a la s ieme recharge
	vector<int> mm; //mm[s] = periode au plutot possible de la s ieme recharge
	vector<int> MM; ////MM[s] = periode au plustard possible de la s ieme recharge
	vector <pair<int, int>>DEC_Prod; //Liste des décisions prise pour construire la solution BSUP pour la production pipeline
	int S = 0; //Nombre de recharge calculé par le pipeline recharge
	Etat_prod FINAL;
	int FINAL_TIME = INFINI; //(*Nil means here Undefined*)
	int temps_restant_st_S__0; //Distance à parcourir par le véhicule pour finir la tournée à la fin de la dernière station
public:
	int Nombre_etats_maximal_prod = 0; //Pour le DPS Pipeline production on calcule le nombre d'états max parmi tous les i
	int Nombre_etats_maximal_recharge = 0;
public:
	Heuristique();
	virtual ~Heuristique();
	list <int> tourne; //notre tournée obtenue avec la fonction pipeline DPS vehicule
	//int cout_recharge;
	vector<vector<int> > Energie;//si Energie[j][h] = -1 alors impossible de retourner au depot avec h H2 en j
	vector<vector<int> >  Temps; //Temps[j][h] = coût min pour aller de j evrs fin avec h dans reservoir si Temps[j][h] = -1 alors impossible de retourner au depot avec h H2 en j

	//initialise une instance
	void allocationTableaux();

	void data(); // calcul des données 
	int distanceEuclidienne(int i, int j);
	int distanceManhattan(int i, int j);

	void Cout_min_prod(); //fonction qui calcule le cout minimal de production int

	void read_instance_in_file(string, string, string);

	void data_prod(string, string num_file);

	//schéma global
	vector<int>Opt_energie;
	vector<int>Opt_temps;

	void dyn_Recharge_temps();
	void dyn_Recharge_energie();

	void Estimation_energie_temps();
	void Tri_bulle();
	//Modifications des instances pour tenir compte de p!=1
	void data_prod_avec_p(string, string);

	//Procédure du Pipeline
	void dyn_Recharge(float alpha, int beta);

	//retourne faux si pas de solution trouvee
	bool intervalles_recharge();
	void extraction_tour(string, float, int);
	void BSUP_Production_flexible(int lamda, string num_file);

	//return faux si echec
	bool dyn_Production_flexible(int num_file, int lamda, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_, int BS);
	Val_nb_State_pipeline Pipeline_T_Reconstruction_procedure(string num_file, double, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_, int BSUP);
	void Prod_Bellman_Update(int i, int q, pair<int, int> DEC, Etat_prod E, int W, bool Filtrage_exacte_pipe_, bool PIPELINE_SANS_filtrage_BSUP_, bool PIPELINE_SANS_filtrage_, bool Filtrage_Heuristique_Pipe_, int K_FILTRAGE_HEURIS_Pipe_);

	bool verifEtatValide_PIPELINE(Etat_prod & ef);
	bool verifEtatValide_PIPELINE_RECHARGE(Etat_Recharg & ef);

#ifdef	OPTIMISATION_TOUR
	string REMOVAL_STATS(string num_file, float BETAA);
	void  INSERT_STATS(string J, string num_file, float BETAA);
	float EVALUATION_TOUR_dps_vehicule(string num_file, float BETAA);
	float EVALUATION_TOUR_learning();
	void OPT_TOUR(int seuil, string num_file, float BETAA);

#endif

};

//========================================================================
// classe SolutionSMEPC : stocke une solution du SMEPC
//

class SolutionSMEPC
{
public:
	SolutionSMEPC();
	virtual ~SolutionSMEPC();
	Heuristique * H;

	vector<bool> stationAvantRecharge;//tableau indicé de 0 a Nbstations stationAvantRecharge[i]= vrai si on recharge juste apres i
	vector<int> quantiteRecharge;//liste des quantites rechargees dans l'ordre de la premiere recharge a la derniere
	vector<bool> periodeProd;//indice de 0 a TMAX-1 : periodeProd[t] = vrai si l usine produit en [t,t+1]
	vector<bool> periodeRecharge;//indice de 0 a TMAX-1 : periodeRecharge[t] = vrai si veh recharge en [t,t+1]

	SolutionSMEPC(Heuristique * inH) : H(inH)
	{
		periodeProd.resize(H->Nombre_de_periode, false);
		periodeRecharge.resize(H->Nombre_de_periode, false);
		stationAvantRecharge.resize(H->Nb_stations + 1, false);
	}

};

//========================================================================
// classe __Etat : stocke un etat pour le programme dyn
//


struct __Etat //Etat
{
	//attention : ne pas changer l'ordre des attributs car on fait des initialisations de __Etat avec {}
	int cit;
	int E;
	int i;
	int delta;
	int CH_h2;
	int _cout;
	int Et_ant; //l"état précédent
	int t_ant;
	int actif = 0;//= 0;
	int i_ant;
};

//========================================================================
// classe Ens_Etat : stocke tous les etats pour le programme dyn
//  Remarque : on utilise un vecteur 1D indicé sur (t,i) : L_etat(t,i) => L_etat(k) où k = (nbStation+2) * t + i
//

class Ens_Etat
{

	int N; // nb de stations + 2 (on compte les depots deb et fin)
	vector<vector<__Etat> > L_etat; //L_etat(t, i) = vecteur qui stocke tous les états pour l'instant (t,i) dans l'ordre lexico

public:

	void init(int N_, int TMAX)
	{
		N = N_;
		int nbMax = TMAX * (N + 1);
		L_etat.resize(nbMax);
		for (int i = 0; i < nbMax; ++i)
		{
			L_etat[i].reserve(1000);
		}
	}

	//renvoie le vecteur d'etats pour l'instant (t,i)
	vector< __Etat> & get(int t, int i)
	{
		return L_etat[N*t + i];
	}

	void set(int t, int i, vector<__Etat> & v)
	{
		L_etat[N*t + i] = v;
	}

	//renvoie l'etat au rang cpt pour l'instant (t,i)
	__Etat & get(int t, int i, int cpt)
	{
		return L_etat[N*t + i][cpt];
	}

	//ajoute un etat a la fin de la liste lie a l'instant (t,i)
	void push(int t, int i, const __Etat & e)
	{
		L_etat[N*t + i].push_back(e);
	}

	//insere l etat ef dans L_etat(t, i) en respectant l'ordre lexico
	void insert(int t, int i, __Etat & ef, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_);

};

//=================================================================================================
// classe principale pour le programme dynamique

class Programdyn
{

public:
	struct NS_indice {
		int cout_NS;
		int ind_NS;

	};
	vector<NS_indice> list_NS_decroissant;


	int compt_etat_supprime = 0;
	SolutionSMEPC sol;
private:

	int etat_present;

	bool L_DEC[8]; //liste de toutes les décisions possibles  //NB_DEC, MOD DEC CHARG
	vector<int>tour;
	vector<int>Etat;
	Ens_Etat tab_Etat;//liste de tous les etats
	vector<__Etat> ETAT_G;

	//vector<vector<int> > L_Etat_;

	//**************data concernant le dépôt***************************
public:
	int st_3 = 0;
private:
	int st_3_t, st_3_i;
	bool B[16];

	//**************data concernant le dépôt***************************
	int depot; //indice du dépôt
	int depot_rech; //indice du dépôt recharge (il est à une distance 1 du depot

  //**************data concernant les décisions***************************
	int NB_DEC; // Nombre de décisions possible
	list <__Etat> list_etat_final_potentiel;
	list<__Etat>::iterator it_etat_final;

	//stocke la position d'un état dans le tableau des états 
	struct ind_it_etat
	{
		int t_ind;
		int i_ind;
		int Et_ind;
	};//sert à récupérer la liste des états de qui ont i=Nbstations
	list <ind_it_etat> indice_list_etat_final_potentiel;
	list<ind_it_etat>::iterator ind_it_etat_final;


	/**********************************************************************ar prog dyn global*/


	float BSUP; //coût BSUP du prog global
	float Cout_prog_dy_global = 1;



public:
	Programdyn();
	virtual ~Programdyn();
	//Génération de l'état initial
	void GENERER_ETAT_INIT(Heuristique& H);//int Etat*, __Etat L_etat*, int L_Etat*
	//initialisation des booléens
	void INIT_BOOLEEN(const __Etat & e, int, Heuristique& H);//__Etat e, int t, int tour*, int rendement*, int tmp*, int Energie*
	//Construction des décisions
	void CONSTRUCT_DECISIONS(__Etat & e, int, int, int, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS);//__Etat e, int t, int L_DEC *

	//Construction des décisions pour le cas particulier de la BSUP
	bool CONSTRUCT_DECISIONS_BSUP(__Etat & e, int, int, int, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_);//__Etat e, int t, int L_DEC *

	//Fonction d'ajout d'un nouvel état dans la liste des états
	void ADD_ETAT(int t, int cit, int E, int i, int delta, int CH_h2, int _cout, int Et_ant, int t_ant, int i_ant, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS); //__Etat*,int*,int*,
	void VEH_maitre_PROD_esclave(Heuristique& H, int, int, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS);
	void VEH_maitre_PROD_esclave_sans_NS(Heuristique& H, int num_file, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS);
	void BSUP_VEH_maitre_PROD_esclave(Heuristique& H, int num_file, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_);
	Val_nb_State_progdyn printf(string, Heuristique &H, double CpuT);

	//renvoie vrai si l'etat est filtre (i.e. etat non faisable)
	//renvoie faux si on peut garder l etat
	//filtrage utilise pour la BSUP : pas de verification le cout !
	bool filtrage_BSUP(int t, const __Etat & e, Heuristique& H);

	//new
	void allocationTableaux(Heuristique H);

	SolutionSMEPC construireSolution(__Etat & e, Heuristique & H);
	//retourne le nb etat actifs et l'indice du dernier
	pair<int, int> verifEtatActif(int t, int i);
	bool verifEtatValide(__Etat & ef, Heuristique& H);

	int verifrecalculedistance(SolutionSMEPC s, Heuristique & H);
	bool verifieusine_ne_produit_pas_durant_recharge(SolutionSMEPC s, Heuristique & H);
	int verifrecalcule_coutusine(SolutionSMEPC s, Heuristique & H);
	int verifduree_total_tournee(SolutionSMEPC s, Heuristique &H);
	int verifderniere_station_recharge(SolutionSMEPC s, Heuristique &H);
	bool verifrecalcule_qte_h2_dans_citerne(SolutionSMEPC s, Heuristique &H);
	bool verifrecalcule_qte_h2_dans_reservoir(SolutionSMEPC s, Heuristique &H);

};

class Creer_instance
{
public:
	bool Creation_instances(int p, int xrectangle, int yrectangle, int xrendement, int xcout, int NB_stations,
		int alpha, int beta, int cout_fixe, int Num_instance, int citerne_initiale, int param_cit, int param_v0, int param_vmax, int param_tmax); //création d'instances avec p = 1
   //void Creation_instances_p(int p); //création d'instances en tenant compte que p peut être > 1


	// 9/03/2022 : LN copier/coller de la fonction precedente => on s'arrange pour avoir exactement 20 periodes (on calule le TMax en fonction des distances
	// et on fixe p pour avoir pile 20 periodes
	bool  Creation_instances_20periodes(int xrectangle, int yrectangle, int xrendement,
		int xcout, int NB_stations, int alpha, int beta,
		int cout_fixe, int Num_instance, int citerne_initiale,
		int param_cit, int param_v0, int param_vmax, int param_tmax);
};

#endif

