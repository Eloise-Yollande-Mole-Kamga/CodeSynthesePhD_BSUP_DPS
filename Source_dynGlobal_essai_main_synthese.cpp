//Programme dynamique global du SMEPC
#include <iostream>
#include <math.h>
//#include "HeaderdynGlobal.h"

//#include "HeaderPipeline.h"
//#include "HeaderProgDyn.h"
//#include "HeaderNS_ProgDyn.h"
//#include "Header_BSUP_prod_pipeline.h"

#include "Header_essai_main_synthese.h"

#include <fstream>
#include <list>
#include <string>
#include<chrono>
#include <algorithm>
#include <tuple>
#include <iterator>
#include <numeric>
#define depot 0
#define depot_rech 0
#define sur_evaluation_temps 1 //car l'estimation  du temps pour finir une tournee est sur donc on supprime tellement d'tats que pour certaines instances on ne cree pas d'etats

using namespace std;
Programdyn::Programdyn()
{
	//ctor
}

Programdyn::~Programdyn()
{
	//dtor
}
SolutionSMEPC::SolutionSMEPC()
{
	//ctor
}

SolutionSMEPC::~SolutionSMEPC()
{
	//dtor
}


//Operateurs
bool operator==(__Etat  f1, __Etat  f2) {
	return std::make_tuple(f1.cit, f1.E, f1.i, f1.delta, f1.CH_h2) == std::make_tuple(f2.cit, f2.E, f2.i, f2.delta, f2.CH_h2);//surcharge opÃ©rateur
};
bool operator<(__Etat  f1, __Etat  f2) {
	return std::make_tuple(f1.cit, f1.E, f1.i, f1.delta, f1.CH_h2) > std::make_tuple(f2.cit, f2.E, f2.i, f2.delta, f2.CH_h2); //surcharge opÃ©rateur
};
bool FonctionQuiCompare(const  Programdyn::NS_indice  s1, const  Programdyn::NS_indice s2)
{
	return (s1.cout_NS < s2.cout_NS ? true : false);
};//new


void Ens_Etat::insert(int t, int i, __Etat & ef, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_)
{
	//if (ef._cout == 160)
	//	cout << "me";

	int k = N * t + i;
	//if ((t == 15)&&(i==7))
		//cout << "me";
	auto it = std::lower_bound(L_etat[k].begin(), L_etat[k].end(), ef); //recherche la position d'un état selon l'ordre lexicographique
	bool etatExistant = (it != L_etat[k].end()) && (ef == *it);
#ifdef VERIF_
	if (etatExistant && !it->actif)
		stopProg("Ens_Etat::insert : on compare avec un etat non actif");
#endif
	if (!etatExistant)
	{
		//int posiion_element_not_egal = std::distance(L_etat[t + 1].begin(), it);
		//decalageEtatpourinsertion(t, pos, posiion_element_not_egal);
		//ajouter_ETAT(t, cit, E, i, delta, CH_h2, _cout, t_ant, Et_ant, posiion_element_not_egal, H);

//Le filtrage heuristique consiste à checher le premier état f1 dont le cout est w1 tel que f1.E==ef.E & |f1.cit-ef.cit|<=smax/K & |f1.CH_h2-ef.CH_h2|<=vmax/K & |T1-T|<=pi_j1/K
		if (Filtrage_Heuristique_)
		{
			int it_etat = 0;
			bool stop = true;
			//int K = 6;//paramètre K utilisé pour réaliser le filtrage heuristique
			while (it_etat < L_etat[k].size() && stop)
			{
				if (ef.E == L_etat[k][it_etat].E && abs(ef.cit - L_etat[k][it_etat].cit) <= (H.smax / K_FILTRAGE_HEURIS_) && abs(ef.CH_h2 - L_etat[k][it_etat].CH_h2) <= (H.vmax / K_FILTRAGE_HEURIS_) && abs(ef.delta - L_etat[k][it_etat].delta) <= ((H.TMAX - H.d__[ef.i] - H.d_[ef.i]) / K_FILTRAGE_HEURIS_))
				{
					if (ef._cout < L_etat[k][it_etat]._cout)
					{
						L_etat[k][it_etat].actif = ef.actif;
						L_etat[k][it_etat].CH_h2 = ef.CH_h2;
						L_etat[k][it_etat].cit = ef.cit;
						L_etat[k][it_etat].delta = ef.delta;
						L_etat[k][it_etat]._cout = ef._cout;
						L_etat[k][it_etat].Et_ant = ef.Et_ant;
						L_etat[k][it_etat].i = ef.i;
						L_etat[k][it_etat].t_ant = ef.t_ant;
						L_etat[k][it_etat].i_ant = ef.i_ant;
						stop = false;
					}
				}
				it_etat++;

			}
			if (it_etat == L_etat[k].size())
				L_etat[k].insert(it, ef);

		}
	if(Filtrage_exacte_)
		L_etat[k].insert(it, ef);

	}
	else
	{
		if (it->_cout > ef._cout)
		{
			int posiion_element_egal = static_cast<int>(distance(L_etat[k].begin(), it));
			L_etat[k][posiion_element_egal]._cout = ef._cout;
			L_etat[k][posiion_element_egal].Et_ant = ef.Et_ant;
			L_etat[k][posiion_element_egal].t_ant = ef.t_ant;
			L_etat[k][posiion_element_egal].i_ant = ef.i_ant;

			//it->_cout = _cout;
			//it->Et_ant = Et_ant;
			//it->t_ant = t_ant;
		}
	}
}


void Programdyn::GENERER_ETAT_INIT(Heuristique& H)//int Etat*, __Etat L_etat*, int L_Etat*
{

	Etat[0] = 0;

	//L_Etat_[0][0] = -1;

	//si je commence   la station 1 c'est comme si j'interdisait a  l'usine de produire a  entre 0 et 1

#ifdef VERIF_
	if (tab_Etat.get(0, 0).size() != 0)
		stopProg("Programdyn::GENERER_ETAT_INIT : L_etat non vide");
#endif
	tab_Etat.push(0, 0, { H.s0, 0, 0, 0, H.v0, 0, -1, -1, 1, -1 });

}

//__Etat e, int t, int tour*, int rendement*, int tmp*, int Energie*
void Programdyn::INIT_BOOLEEN(const __Etat & e, int t, Heuristique& H) //p OK
{
	/*
	B[0] = 0;
	//bool B1, B2, B3, B4, B5, B6, B7, B8, B9, B10, B11, B12, B13, B14;
	B[1] = (e.delta > t);//B1 =
	B[2] = (e.delta < t);//B2 =
	B[3] = ((e.delta + H.d[e.i][depot]) > t); // B3 =
	B[4] = (B[1] || B[3]); //B4 : t != delta
	B[5] = ((e.cit + H.rendement[t]) <= H.smax); // B5 : on peut produire en [t,t+1]
	int a = tour[e.i];
	int b = tour[e.i + 1];
	B[6] = ((e.CH_h2 - H.E[a][b]) >= 0); // B6 =
	B[7] = ((t + H.d[a][depot_rech] + H.d[depot_rech][b] + tmp_h2) < H.TMAX); //=0// B7 =
	B[12] = ((e.E == 0) || (B[5] == true)); // B12 =
	B[13] = ((e.E == 1) || (B[5] == true)); //  B13 =
	B[14] = ((t + H.d[depot_rech][b] + tmp_h2) >= H.TMAX);// B14 =
	B[8] = (B[12] && !B[14]); // B8 =
	B[9] = (B[13] && !B[14]); // B9 =
	B[10] = (B[12] && B[7]); //B10 =
	B[11] = (B[13] && B[7]); // B11 =
	B[15] = (e.i >= 0 && e.i <= H.Nb_stations && e.CH_h2 - H.E_i[e.i] - H.E_i0[e.i+1] >= 0);*/
	//B[16] = (e.delta + H.d[tour[e.i]][tour[e.i + 1]] <= H.TMAX && e.delta + H.d[tour[e.i]][tour[e.i + 1]] >= 0);


	//on ecrit ici les conditions avec une duree de periode = 1 (on precise par MODIF en commentaire si la condition doit changer quand la duree de la periode > 1)
	//Decisions article
	B[4] = H.p * t <= e.delta && e.delta < H.p*(t + 1); // e.delta == t -- p OK

	B[7] = e.CH_h2 - H.E_i[e.i] - H.E_i0[e.i + 1] >= 0; //p OK
	B[9] = H.p*(t + 1) + H.p + H.d_0i[e.i + 1] + H.d__[e.i + 1] <= H.TMAX; //p OK
	if (t < H.Nombre_de_periode)
		B[1] = (e.cit + H.rendement[t] <= H.smax); // B1 : on peut produire en [t,t+1] -- p OK
	B[2] = B[4] && B[7] && (e.delta + H.d_i[e.i] >= H.p*(t + 1)); //p OK
	B[3] = (e.delta < H.p*t) && B[9]; //p OK

	B[5] = max(H.p*(t + 1), e.delta + H.d_i0[e.i]) + H.d_0i[e.i + 1] + H.p + H.d__[e.i + 1] <= H.TMAX; //p OK
	B[6] = (e.delta < H.p*t) || (e.delta >= H.p*(t + 1)); // p OK
	B[8] = (e.delta + H.d_i0[e.i] <= H.p*t); //p OK
	B[10] = (H.E_i0[e.i + 1] + H.E_0i[e.i + 1] <= min(H.vmax, e.cit + e.CH_h2 - H.E_i0[e.i])); //p OK
	B[11] = e.delta + H.d_i[e.i] <= H.TMAX;

}

// k : position de e dans la liste tab_etat(t,e.i)
void Programdyn::CONSTRUCT_DECISIONS(__Etat & e, int t, int NB_DEC, int k, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS) // p OK
{
	//D'après le papier : les decisions 3, 5, 7 sont impossible => L_DEC[3], L_DEC[5], L_DEC[7] zesteront FAUX

	//instant courant :
	int tcour = t, icour = e.i;

	//instant suiavnt : 
	int tsuiv = t, isuiv = e.i;

	//init a faux
	for (int j = 0; j < NB_DEC; j++)
	{
		L_DEC[j] = false;
	}

	int cpt = 0;
	//----------------------------------------------------------------------
	//activation de L_DEC[0] ? (a,b,c) = (0,0,0) : 
	// l'usine ne produit pas durant [p*t,p*(t+1)[ et le véhicule poursuit vers la prochaine station  
	if (B[4]) //si e.delta = t, 
	{
		if (B[7] && B[11])// on peut continuer si on a assez d'energie (sinon pas de creation d etat)
		{					//B[7] = e.CH_h2 - H.E_i[e.i] - H.E_i0[e.i + 1] >= 0;
							//B[11] = e.delta + H.d_i[e.i] <= H.TMAX;
			// (t, e.i) --> (t+1, e.i+1)

			isuiv = icour + 1;

			if (H.p * t <= e.delta + H.d_i[e.i] && e.delta + H.d_i[e.i] < H.p*(t + 1)) //MODIF : EGALE e.delta + H.d_i[e.i] == t
			{
				tsuiv = tcour;
				ADD_ETAT(tsuiv, e.cit, e.E, isuiv, e.delta + H.d_i[e.i], e.CH_h2 - H.E_i[icour], e._cout + H.beta * H.d_i[e.i], tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_,BS);
				cpt++;
			}
			else
			{
				tsuiv = tcour + 1;
				ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta + H.d_i[e.i], e.CH_h2 - H.E_i[icour], e._cout + H.beta * H.d_i[e.i], tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
				cpt++;
			}
		}
	}
	else // e.delta < t ou e.delta > t
	{
		if ((B[8] && B[9]) || !B[8])// (e.delta + H.d_i0[e.i] <= t) 
		{							// e.delta >= H.p * (t + 1) || e.delta < H.p*(t)
			tsuiv = tcour + 1;
			isuiv = icour;

			ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta, e.CH_h2, e._cout, tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
			cpt++;

		}
	}

	//----------------------------------------------------------------------
	//activation de L_DEC[1] ? (a,b,c) = (0,0,1) : 
	// l'usine ne produit pas durant [p*t,p*(t+1)[ et le vehicule va recharger pendant [p*t,p*(t+1)[

	if (B[8] && B[10]) //(e.delta + H.d_i0[e.i] <= t) et (H.E_i0[e.i + 1] + 
					   //H.E_0i[e.i + 1] <= min(H.vmax, e.cit + e.CH_h2 + -H.E_i0[e.i]));
	{
		tsuiv = tcour + 1;
		isuiv = icour + 1;
		//#ifdef VERIF_
		//		if (isuiv == H.Nb_stations + 1)
		//			stopProg(" Nous sommes à une fin possible");
		//#endif

				//A t-on t + 1 ?
		if (H.p*(t + 1) + H.d_0i[e.i + 1] <= H.TMAX) // a verifier //- H.p?
		{
			int recharge_veh = min(e.cit, H.vmax - e.CH_h2 + H.E_i0[icour]);
			int qte_pour_finir_tour = H.E_i0[isuiv] + H.E__[isuiv] + H.v0;//qte_pour_finir_tour=Quantité de h2 pour finir le tour en partant de l'usine-->isuiv...
			recharge_veh = min(recharge_veh, max(0, qte_pour_finir_tour - e.CH_h2 + H.E_i0[icour]));
			if (recharge_veh < 0 || recharge_veh > e.cit || recharge_veh > H.vmax - (e.CH_h2 - H.E_i0[icour]))
				stopProg("!!!!!!!!!!!!!!!Programdyn::CONSTRUCT_DECISIONS : Erreur recharge_veh<0 ou >cit");
			//ADD_ETAT(tsuiv, e.cit - recharge_veh, 0, isuiv, H.p*(t + 1) + H.d_0i[e.i + 1],
			//min(H.vmax - H.E_0i[isuiv], e.cit + e.CH_h2 - H.E_i0[icour] - H.E_0i[isuiv]), e._cout + H.beta * (H.p*(t + 1) + H.d_0i[isuiv] - e.delta), tcour, k, icour, H);
			ADD_ETAT(tsuiv, e.cit - recharge_veh, 0, isuiv, H.p*(t + 1) + H.d_0i[e.i + 1],
				e.CH_h2 - H.E_i0[icour] + recharge_veh - H.E_0i[isuiv], e._cout + H.beta * (H.p*(t + 1) + H.d_0i[isuiv] - e.delta), tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);

			cpt++;
		}

		//cas particulier remarque 5 ??
	}

	//----------------------------------------------------------------------
	//activation de L_DEC[2] ? (a,b,c) = (0,1,0) : 
	// l'usine ne produit pas pendant [p*t,p*(t+1)[ et le véhicule se dirige vers l'usine pour recharger


	if (B[4] && B[5]) //si e.delta = t et (max((t + 1), e.delta + H.d_i0[e.i])
					 //+ H.d_0i[e.i + 1] + H.d__[e.i + 1] +1<= H.TMAX);

	{
		tsuiv = tcour + 1;
		isuiv = icour;
		ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta, e.CH_h2, e._cout, tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		cpt++;

	}

	//----------------------------------------------------------------------
	//activation de L_DEC[3] ? (0,1,1)
	//impossible 

	//----------------------------------------------------------------------
	// activation de L_DEC[4] ? (1,0,0)
	//  l'usine produit pendant [p*t,p*(t+1)[ et le véhicule continue sa route 
	//

	if (B[1])// B[1] : il reste dans la citerne suffisement de place pour produire
	{
		if (B[2] && B[11]) //((e.cit + H.rendement[t]) <= H.smax); // B1 : on peut produire en [t,t+1]
						  //B[2] = ((e.delta == t) && B[7] && (e.delta + H.d_i[e.i] > t))

		{

			tsuiv = tcour + 1;
			isuiv = icour + 1;
			//#ifdef VERIF_
			//		if (isuiv == H.Nb_stations + 1)
			//			stopProg(" Nous sommes à une fin possible");
			//#endif
			ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta + H.d_i[e.i], e.CH_h2 - H.E_i[e.i], e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.beta * H.d_i[e.i], tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
			cpt++;

		}

		if (B[3]) //B[3] = ((e.delta < t) && B[9]);
		{
			tsuiv = tcour + 1;
			isuiv = icour;
			ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta, e.CH_h2, e._cout + (H.Cf*(1 - e.E) + H.coutv[t]), tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
			cpt++;
		}

		if (e.delta >= H.p*(t + 1)) //ajout au 17/04/2020
		{
			tsuiv = tcour + 1;
			isuiv = icour;
			ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta, e.CH_h2, e._cout + (H.Cf*(1 - e.E) + H.coutv[t]), tcour, k, icour, H,Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
			cpt++;
		}
	}
	//----------------------------------------------------------------------
	//activation de L_DEC[5] ? (1,0,1)
	//impossible

	//----------------------------------------------------------------------
	//activation de L_DEC[6] ? (1,1,0)
	// l'usine produit durant [p*t,p*(t+1)[ et le vehicule decide d'aller de i vers l'usine pour recharger

	if (B[4] && B[1] && B[5]) //B[4]= (e.delta = t), et B[5] = (max((t + 1), e.delta + 
						 //H.d_i0[e.i]) + H.d_0i[e.i + 1] + H.d__[e.i + 1] <= H.TMAX);

	{
		tsuiv = tcour + 1;
		isuiv = icour;
		ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta, e.CH_h2, e._cout + (H.Cf*(1 - e.E) + H.coutv[t]), tcour, k, icour, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		cpt++;
	}
	//----------------------------------------------------------------------
	//activation de L_DEC[7] ?
	//impossible



}

// k : position de e dans la liste tab_etat(t,e.i)
bool Programdyn::CONSTRUCT_DECISIONS_BSUP(__Etat & e, int t, int NB_DEC, int k, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_)//__Etat e, int t, int L_DEC *
{

	bool ok = false; // true => on a reussi a creer un etat
	bool interdit;

	//D'après le papier : les decisions 3, 5, 7 sont impossible => L_DEC[3], L_DEC[5], L_DEC[7] zesteront FAUX

	__Etat bestEtat, ec;
	bestEtat.i = -1;

	double estimation;
	double bestEtimation = INFINI;

	//instant courant :
	int tcour = t, icour = e.i;

	//instant suiavnt : 
	int tsuiv = t, isuiv = e.i;

	//init a faux
	for (int j = 0; j < NB_DEC; j++)
	{
		L_DEC[j] = false;
	}
	//==========================================================================
	// a : usine produit (1) ou pas (0), 
	// b : véhicule continue (0) vers prochaine station ou pas (1)
	// c : vehicule recharge en [t,t+1] (1) ou pas (0)
	// e.delta = t => le véhicule est a la station i : il doit décider de continuer ou recharger
	// e.delta < t => le véhiule a passé la station i, il va ou attend a l'usine
	// e.delta > t => le véhiule se dirige vers la station, il y a arrivera en delta

	//----------------------------------------------------------------------
	//activation de L_DEC[0] ? (a,b,c) = (0,0,0) : 
	// l'usine ne produit pas durant [t,t+1] et le véhicule poursuit vers la prochaine station 


	//la quantite de H2 dans le vehicule suffit (estimation optimiste) pour finir le tour sans recharge
	bool H2_suffisant = e.CH_h2 >= H.Energie[e.i][e.CH_h2];

	if (B[4]) //si e.delta = t, 
	{
		//je suis a la station i , je decide d'avancer vers i+1 => on veut assez dans la citerne pour faire le plein après i+1
		interdit = !H2_suffisant && (e.cit < H.vmax - e.CH_h2 + H.E_i[e.i] + H.E_i0[e.i + 1]);

		if (!interdit && B[7] && B[11])// on peut continuer si on a assez d'energie (sinon pas de creation d etat)
		{
			// (t, e.i) --> (t+1, e.i+1)
			tsuiv = tcour + 1;
			isuiv = icour + 1;
			if (H.p * t <= e.delta + H.d_i[e.i] && e.delta + H.d_i[e.i] < H.p*(t + 1)) //MODIF : EGALE e.delta + H.d_i[e.i] == t < t
			{
				tsuiv = tcour;
				ec.E = e.E;
			}
			else
			{
				tsuiv = tcour + 1;
				ec.E = 0;
			}



			ec.CH_h2 = e.CH_h2 - H.E_i[icour];
			ec.cit = e.cit;
			ec.delta = e.delta + H.d_i[e.i];
			ec.i = isuiv;

			int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv

			estimation = e._cout + H.beta * H.d_i[e.i] + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];

			//ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta + H.d_i[e.i], e.CH_h2 - H.E_i[icour], e._cout + H.beta * H.d_i[e.i], tcour, k, icour, H);
			if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
			{
				bestEtat = ec;
				bestEtat._cout = e._cout + H.beta * H.d_i[e.i];
				bestEtimation = estimation;
			}

		}
	}
	else // e.delta < t ou e.delta > t : si e.delta + d_i <= t => le vehicule a decide de recharger entre i et i+1 et il est en attente a l usine
	{
		//on a passe la station i et on est en route vers l'usine && la citerne n'est pas rempli pour faire le plein
		interdit = (e.delta < H.p*t && 1 * t - e.delta <= H.d_i0[e.i]) && (e.cit < H.vmax - e.CH_h2 + H.E_i0[e.i]);

		// le véhicule est en attente a l'usine => on doit produire (on interdit usine + véhicule qui attendent)
		bool interdit2 = e.delta < H.p*t && 1 * t - e.delta >= H.d_i0[e.i];


		if (!interdit && !interdit2 && ((B[8] && B[9]) || !B[8])) // (e.delta + H.d_i0[e.i] <= t)
		{
			tsuiv = tcour + 1;
			isuiv = icour;

			//ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta, e.CH_h2, e._cout, tcour, k, icour, H);


			ec.CH_h2 = e.CH_h2;
			ec.cit = e.cit;
			ec.E = 0;
			ec.delta = e.delta;
			ec.i = isuiv;


			int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv

			estimation = e._cout + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];


			if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
			{
				bestEtat = ec;
				bestEtat._cout = e._cout;
				bestEtimation = estimation;
			}

		}
	}

	//----------------------------------------------------------------------
	//activation de L_DEC[1] ? (a,b,c) = (0,0,1) : 
	// l'usine ne produit pas durant [t,t+1] et le vehicule va recharger pendant [t,t+1]



	if (B[8] && B[10]) //(H.d_i0[e.i] <= t - e.delta) et (H.E_i0[e.i + 1] + 
					   //H.E_0i[e.i + 1] <= min(H.vmax, e.cit + e.CH_h2 + -H.E_i0[e.i]));
	{

		//pourquoi obliger le vehicule a se recharger completement ??
		//interdit = ( t - e.delta == H.d_i0[e.i]) && (e.cit < H.vmax - e.CH_h2 + H.E_i0[e.i]);

		//on se recharge si on a plus dans la citerne que la quantite d'H2 dépensée pour faire le détour
		interdit = e.cit < H.E_i0[e.i] + H.E_0i[e.i + 1];

		if (!interdit && H.p*(t + 1) + H.d_0i[e.i + 1] <= H.TMAX) // a verifier //-H.p?

		{

			tsuiv = tcour + 1;
			isuiv = icour + 1;
			//#ifdef VERIF_
			//		if (isuiv == H.Nb_stations + 1)
			//			stopProg(" Nous sommes à une fin possible");
			//#endif

					//A t-on t + 1 ?
			//ADD_ETAT(tsuiv, e.cit - min(e.cit, H.vmax - e.CH_h2 + H.E_i0[icour]), 0, isuiv, t + 1 + H.d_0i[e.i + 1],
				//min(H.vmax - H.E_0i[isuiv], e.cit + e.CH_h2 - H.E_i0[icour] - H.E_0i[isuiv]), e._cout + H.beta * (t + 1 + H.d_0i[isuiv] - e.delta), tcour, k, icour, H);
			int recharge_veh = min(e.cit, H.vmax - e.CH_h2 + H.E_i0[icour]);
			int qte_pour_finir_tour = H.E_i0[isuiv] + H.E__[isuiv] + H.v0;//qte_pour_finir_tour=Quantité de h2 pour finir le tour en partant de l'usine-->isuiv...
			recharge_veh = min(recharge_veh, max(0, qte_pour_finir_tour - e.CH_h2 + H.E_i0[icour]));
			if (recharge_veh < 0 || recharge_veh > e.cit || recharge_veh > H.vmax - (e.CH_h2 - H.E_i0[icour]))
				stopProg("!!!!!!!!!!!!!!!Programdyn::CONSTRUCT_DECISIONS : Erreur recharge_veh<0 ou >cit");


			ec.CH_h2 = e.CH_h2 - H.E_i0[icour] + recharge_veh - H.E_0i[isuiv];
			ec.cit = e.cit - recharge_veh;//, 0, isuiv, H.p*(t + 1) + H.d_0i[e.i + 1];
			ec.E = 0;
			ec.delta = t + 1 + H.d_0i[e.i + 1];
			ec.i = isuiv;


			int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv

			estimation = e._cout + H.beta * (t + 1 + H.d_0i[isuiv] - e.delta) + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];


			if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
			{
				bestEtat = ec;
				bestEtat._cout = e._cout + H.beta * (t + 1 + H.d_0i[isuiv] - e.delta);

				bestEtimation = estimation;
			}
		}
		//cas particulier remarque 5 ??
	}

	//----------------------------------------------------------------------
	//activation de L_DEC[2] ? (a,b,c) = (0,1,0) : 
	// l'usine ne produit pas pendant [t,t+1] et le véhicule se dirige vers l'usine pour recharger (ou il attend a l'usine)

	//interdit si on ne peut pas faire le plein
	interdit = (B[4]) && (e.cit < H.vmax - e.CH_h2 + H.E_i0[e.i]);

	if (!interdit && B[4] && B[5]) //si e.delta = t et (max((t + 1), e.delta + H.d_i0[e.i])
					 //+ H.d_0i[e.i + 1] + H.d__[e.i + 1] +1<= H.TMAX);

	{
		tsuiv = tcour + 1;
		isuiv = icour;

		//ADD_ETAT(tsuiv, e.cit, 0, isuiv, e.delta, e.CH_h2, e._cout, tcour, k, icour, H);

		ec.CH_h2 = e.CH_h2;
		ec.cit = e.cit;
		ec.E = 0;
		ec.delta = e.delta;
		ec.i = isuiv;

		int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv

		estimation = e._cout + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];

		// <= pour favoriser ce cas s'il ne coute pas plus cher que la non recharge
		if (estimation <= bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
		{
			bestEtat = ec;
			bestEtat._cout = e._cout;
			bestEtimation = estimation;
		}
	}

	//----------------------------------------------------------------------
	//activation de L_DEC[3] ? (0,1,1)
	//impossible 

	//----------------------------------------------------------------------
	// activation de L_DEC[4] ? (1,0,0)
	//  l'usine produit pendant [t,t+1] et le véhicule continue sa route 
	//
	if (B[1])// B[1] : il reste dans la citerne suffisement de place pour produire
	{
		if (B[2] && B[11]) //((e.cit + H.rendement[t]) <= H.smax); // B1 : on peut produire en [t,t+1]
						  //B[2] = ((e.delta == t) && B[7] && (e.delta + H.d_i[e.i] > t))

		{
			//bool B = (e.CH_h2 - H.E_i[e.i] - H.E_i0[e.i + 1] >= 0);
			//bool B2 = (Programdyn::B[1] && Programdyn::B[2]);
			tsuiv = tcour + 1;
			isuiv = icour + 1;
			//#ifdef VERIF_
			//		if (isuiv == H.Nb_stations + 1)
			//			stopProg(" Nous sommes à une fin possible");
			//#endif
			//ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta + H.d_i[e.i], e.CH_h2 - H.E_i[e.i], e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.beta * H.d_i[e.i], tcour, k, icour, H);



			ec.CH_h2 = e.CH_h2 - H.E_i[e.i];
			ec.cit = e.cit + H.rendement[t];
			ec.E = 1;
			ec.delta = e.delta + H.d_i[e.i];
			ec.i = isuiv;

			int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv


			estimation = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.beta * H.d_i[e.i] + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];


			if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
			{
				bestEtat = ec;
				bestEtat._cout = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.beta * H.d_i[e.i];
				bestEtimation = estimation;
			}

		}

		if (B[3] || e.delta >= H.p*(t + 1)) //ajout au 17/04/2020) //B[3] = ((e.delta < t) && B[9]);
		{
			tsuiv = tcour + 1;
			isuiv = icour;

			//ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta, e.CH_h2, e._cout + (H.Cf*(1 - e.E) + H.coutv[t]), tcour, k, icour, H);



			ec.CH_h2 = e.CH_h2;
			ec.cit = e.cit + H.rendement[t];
			ec.E = 1;
			ec.delta = e.delta;
			ec.i = isuiv;

			int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv

			estimation = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];

			if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
			{
				bestEtat = ec;
				bestEtat._cout = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]);
				bestEtimation = estimation;
			}

		}

	}

	//----------------------------------------------------------------------
	//activation de L_DEC[5] ? (1,0,1)
	//impossible

	//----------------------------------------------------------------------
	//activation de L_DEC[6] ? (1,1,0)
	// l'usine produit durant [t,t+1] et le vehicule decide d'aller de i vers l'usine pour recharger

	if (B[4] && B[1] && B[5]) //B[4]= (e.delta = t), et B[5] = (max((t + 1), e.delta + 
						 //H.d_i0[e.i]) + H.d_0i[e.i + 1] + H.d__[e.i + 1] <= H.TMAX);

	{
		tsuiv = tcour + 1;
		isuiv = icour;

		//ADD_ETAT(tsuiv, e.cit + H.rendement[t], 1, isuiv, e.delta, e.CH_h2, e._cout + (H.Cf*(1 - e.E) + H.coutv[t]), tcour, k, icour, H);


		ec.CH_h2 = e.CH_h2;
		ec.cit = e.cit + H.rendement[t];
		ec.E = 1;
		ec.delta = e.delta;
		ec.i = isuiv;

		int energ_prod = max(0, H.Energie[isuiv][ec.CH_h2] - ec.cit - ec.CH_h2); //énergie à produire a partir de isuiv


		estimation = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]) + H.VAL[tsuiv][ec.E][energ_prod] + H.beta * H.Temps[isuiv][ec.CH_h2];


		if (estimation < bestEtimation && !filtrage_BSUP(tsuiv, ec, H))
		{
			bestEtat = ec;
			bestEtat._cout = e._cout + (H.Cf*(1 - e.E) + H.coutv[t]);
			bestEtimation = estimation;
		}

	}
	//----------------------------------------------------------------------
	//activation de L_DEC[7] ?
	//impossible

	if (bestEtat.i != -1)
	{
		ok = true;

		bestEtat.t_ant = tcour;
		bestEtat.i_ant = icour;
		bestEtat.Et_ant = k;

		tab_Etat.insert(tcour + 1, bestEtat.i, bestEtat, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_);

#ifdef VERIF_
		if (!verifEtatValide(bestEtat, H))
			stopProg("Programdyn::CONSTRUCT_DECISIONS_BSUP  !! on a cree un etat non valide");

#endif
	}

	return ok;
}


//pos nb d etats presents en t+1 dans L_Etat
void Programdyn::ADD_ETAT(int t, int cit, int E, int i, int delta, int CH_h2, int _cout, int  t_ant, int Et_ant, int i_ant, Heuristique& H, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS)// p OK
{

	__Etat  ef;
	ef.cit = cit;
	ef.E = E;
	ef.i = i; //indice de la premire station visite
	ef.delta = delta;
	ef.CH_h2 = CH_h2;
	ef._cout = _cout;
	ef.Et_ant = Et_ant;
	ef.t_ant = t_ant;
	ef.actif = 1;
	ef.i_ant = i_ant;



#ifdef VERIF_
	if (!verifEtatValide(ef, H))
		stopProg("!! Programdyn::ADD_ETAT : on a cree un etat non valide");
#endif

	//si etat non valide alors on retourne pos (rien a faire)
	if (!verifEtatValide(ef, H))
		return;
	//if (ef._cout == 160)
	//	cout << "me";
	if (ef.i == H.Nb_stations + 1)
	{
		//si on est arrive a la fin on ajoute l etat sans faire l'ordre lexico	
		//L_etat[t + 1].push_back(ef);
//#ifdef VERIF_
//		stopProg("Fin possible");
//#endif
		if (ef.cit >= H.s0)
		{
			if (ef._cout <= BS)
				tab_Etat.push(t, i, ef);
		}
		else
		{
			ef._cout = ef._cout + H.VAL[t][ef.E][H.s0 - ef.cit];
			if (ef._cout <= BS)
			{
				ef.cit = H.s0; //Ici l'usine peut produire plus que s0, la valeur ef.cit est la quantité minimal d'hydrogène dans la citerne
				tab_Etat.push(t, i, ef);
			}
		}

	}
	else
	{
		// H.Temps[i][x]  = temps minimal pour aller de i vers le depot fin avec x H2 dans le réservoir (en partant de i) = -1 si impossible d'aller recharger avec x H2 en i
		// H. Energie[i][x] = eneegie minimale pour aller de i vers depot fin avec x H2 dans le réservoir (en partant de i) = -1 si impossible d'aller recharger avec x H2 en i


		//si energie ou temps fait -1 alors il est impossible de finir le tour a partir de i avec CH_h2 H2
		if (H.Energie[i][CH_h2] != -1 && H.Temps[i][CH_h2] != -1)
		{

			int energ_prod = max(0, H.Energie[i][CH_h2] - cit - CH_h2 + H.s0); //énergie totale à produire
			int energ_prod_veh = max(0, H.Energie[i][CH_h2] - cit - CH_h2);
			int energ_prod_cit = max(0, H.s0 - cit);
			energ_prod = max(energ_prod, energ_prod_cit + energ_prod_veh);
			//Si on est sur la dernière station et qu'on peut finir sans produire alors il ne faut pas "+H.s0"
			/*if ((i == H.Nb_stations) && (delta == t * H.p))
			{
				if (cit + CH_h2 - (H.E_i0[i] + H.E_0i[i + 1]) >= H.s0 + H.v0)
					energ_prod = max(0, H.Energie[i][CH_h2] - cit - CH_h2);
			}*/
			//peut on vraiment connaitre ces quantités ? car on ne sait pas combien de temps le véhicule attend, la citerne est peut être en production
			//LN : je pense que h2_dansVehicule n'est connu que dans le cas où c = 1 et c'est déjà testé dans les décisions => on ne reteste pas ici (?)
			//int h2_apresRecharge = min(H.vmax, cit + ef.CH_h2 - H.E_i0[i]);//qte de h2 dans le reservoir quand le vehicule quitte l'usine [cas t > delta]
			//int h2_dansVehicule = h2_apresRecharge - H.E[0][i + 1];



			bool tempsOK = ((H.p*t <= delta) && (delta + H.Temps[i][CH_h2] <= H.TMAX)) || // le véhicule est sur la station i, il est arrive sur i a la date delta
				((H.p*t > delta) && (H.p*t + H.d_0i[i + 1] + H.d__[i + 1] <= H.TMAX)); //le vehicule entre i et usine, eventuellement en attente {RQ : + p pour la duree du chargement ??}
																			 //(on utilise d__ car on ne sait pas combien de temps on attend : le citerne est peutêtre en train de se remplir

			bool coutOK = (_cout + H.VAL[t][E][energ_prod] + (H.beta * H.Temps[i][CH_h2]) <= BS);

			bool energieOK = ((H.p*t <= delta) && (cit + H.q__[t] + CH_h2 >= H.Energie[i][CH_h2])) ||
				((H.p*t > delta) && (cit + H.q__[t] + CH_h2 >= H.E_0i[i + 1] + H.E__[i + 1]));

			if(PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_)
				coutOK = true;

			bool filtrage = tempsOK && coutOK && energieOK;// 



#ifdef VERIF_
			if (ef.i > H.Nb_stations)
				stopProg("Programdyn::ADD_ETAT: arrive ici on devait avoir ef.i <= H.Nb_stations");
#endif

			if (PROGRAMME_DYNAMIQUE_SANS_filtrage_)
				filtrage = true;
			

			if (filtrage)//filtrage
			{

				tab_Etat.insert(t, i, ef, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_);
				
			}//fin if filtrage

		}
		//if (H.Energie[i][CH_h2] == -1 || H.Temps[i][CH_h2] == -1)tab_Etat.insert(t, i, ef);

	}//fin else (i != nbStation+1)

}

//renvoie vrai si l'etat est filtre (i.e. etat non faisable)
//renvoie faux si on peut garder l etat
//filtrage utilise pour la BSUP : pas de verification le cout !
bool Programdyn::filtrage_BSUP(int t, const __Etat & e, Heuristique& H)
{

	bool tempsOK = ((t <= e.delta) && (e.delta + H.Temps[e.i][e.CH_h2] <= H.TMAX)) || // le véhicule est sur la station i, il est arrive sur i a la date delta
		((t > e.delta) && (t + H.d_0i[e.i + 1] + H.d__[e.i + 1] <= H.TMAX)); //le vehicule entre i et usine, eventuellement en attente 
																	 //(on utilise d__ car on ne sait pas combien de temps on attend : le citerne est peutêtre en train de se remplir


	bool energieOK = ((t <= e.delta) && (e.cit + H.q__[t] + e.CH_h2 >= H.Energie[e.i][e.CH_h2])) ||
		((t > e.delta) && (e.cit + H.q__[t] + e.CH_h2 >= H.E_0i[e.i + 1] + H.E__[e.i + 1]));



	return !tempsOK || !energieOK;
}


//affiche les indices interessants de L_etat[t,i]
pair<int, int> Programdyn::verifEtatActif(int t, int i)
{
	int nbEtat = 0, erreur = 0;
	int indiceDernier = -1;

	vector<__Etat> l_etat = tab_Etat.get(t, i);

	for (unsigned int k = 0; k < ETAT_MAX && k < l_etat.size(); ++k)
	{
		if (l_etat[k].actif)
		{
			nbEtat++;
			if (indiceDernier != -1)
				erreur = 1;
		}
		else
		{
			if (indiceDernier == -1)
				indiceDernier = k - 1;
		}

	}

	std::cout << "nb etats pour la date " << t << " = " << nbEtat << endl;
	if (erreur == 1)
	{
		std::cout << "un etat actif apres un etat non actif" << endl;
		//cin >> erreur;
	}

	return { nbEtat,indiceDernier };
}

bool Programdyn::verifEtatValide(__Etat & ef, Heuristique & H) // p OK
{
	//bool b = ((ef._cout == 5) && (ef.delta == 5) && (ef.CH_h2 == 4));

	bool ok = true;

	if (ef.CH_h2 < 0 || ef.CH_h2 > H.vmax)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.CH_h2 < 0 || ef.CH_h2 > H.vmax" << endl;
	}

	if (ef.cit < 0 || ef.cit > H.smax)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.cit < 0  || ef.cit > H.smax" << endl;
	}

	if (ef.delta < 0 || ef.delta > H.TMAX)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.delta < 0 || ef.delta > H.TMAX" << endl;
	}

	if (ef.E != 0 && ef.E != 1)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.E != 0 && ef.E != 1" << endl;
	}

	if (ef.Et_ant < 0 && ef.E > ETAT_MAX)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.Et_ant < 0 && ef.E > ETAT_MAX" << endl;
	}
	if (ef.i < 0 && ef.i > H.Nb_stations + 1)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.i < 0 && ef.i > H.Nb_stations + 1" << endl;
	}

	if (ef.t_ant < 0 && ef.t_ant > H.Nombre_de_periode)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.t_ant < 0 && ef.t_ant > H.TMAX" << endl;
	}

	if (ef._cout < 0)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef._cout < 0" << endl;
	}

	//si i est une vraie station (pas depot) alors il faut assez de h2 pour aller a la prochaine station ou a l usine
	if (ef.i >= 1 && ef.i <= H.Nb_stations && ef.CH_h2 == 0)
	{
		ok = false;
		cout << "Programdyn::verifEtatValide ef.i >= 1 && ef.i <= H.Nb_stations && ef.CH_h2 == 0" << endl;
	}

	return ok;
}

void Programdyn::VEH_maitre_PROD_esclave(Heuristique& H, int num_file, int NS, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_,int BS)
{
	int POS = 0;

	if (BS > 0)//(H.BSUP[num_file] > 0)
	{
		BSUP = BS;// H.BSUP[num_file];
	}
	else
	{
		BSUP = 2 * BSUP_MAX;
	}

	NB_DEC = 8; // Nombre de dcisions possible


	list_NS_decroissant.clear();
	std::cout << "********deb*****" << endl;

	for (int i = 0; i < H.Nombre_de_periode; i++)
	{
		//DECISIONS[0][0] = 0;
		Etat[i] = 0;
	}




	//std::cout << "Le nombre d'etat cree a t=" << t << "est : " << pos - compt_etat_supprime << endl;

	for (int t = 0; t <= H.Nombre_de_periode; ++t)
	{
		for (int i = 0; i <= H.Nb_stations; ++i)
		{
			compt_etat_supprime = 0;
			__Etat e;
			vector<__Etat> l_etats = tab_Etat.get(t, i); //liste des etats a la date (t,i)

			//parcourt tous les etats a l "instant" (t,i)
			for (unsigned int i_cmp = 0; i_cmp < l_etats.size(); i_cmp++)
			{
				e = l_etats[i_cmp];

				INIT_BOOLEEN(e, t, H);//__Etat e, int t, int tour*, int rendement*, int tmp*, int Energie* //, &(tour[0]), &(rendement[0]), &(d[0][0]), &(H.E[0][0])

				CONSTRUCT_DECISIONS(e, t, NB_DEC, i_cmp, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);

			}

			//NS procedure debut
			vector<__Etat> l_etatsSuivant;
			int isuiv = i + 1, tsuiv = t; //instant suivant (t,i)
			if (i == H.Nb_stations)
			{
				tsuiv = t + 1;
				isuiv = 0;
			}

			l_etatsSuivant = tab_Etat.get(tsuiv, isuiv);


			int nbEtat_suiv = static_cast<int>(l_etatsSuivant.size());
			if (nbEtat_suiv > NS)
			{
				int nbEtatStockes = 0;

				vector <int> TAB_ETAT_BY_Cout; //Tableau TAB_ETAT_BY_Cout[u] contenant le nombre d'état de cout u avec u = (cout etat+ cout estimer de letat( 
				TAB_ETAT_BY_Cout.resize(COUT_MAXIMAL_NS, 0);
				for (int i_cmp = 0; i_cmp < nbEtat_suiv; i_cmp++) //new cmp devient 10
				{
					e = l_etatsSuivant[i_cmp];
					int energ_prod = max(0, H.Energie[tour[e.i]][e.CH_h2] - e.cit - e.CH_h2) + H.s0; //énergie à produire

					if (H.VAL[t][e.E][energ_prod] != INFINI)
					{
						TAB_ETAT_BY_Cout[e._cout + (H.VAL[t][e.E][energ_prod]) + (H.beta * H.Temps[tour[e.i]][e.CH_h2])]++;
						nbEtatStockes++;
					}

				}
				int s = 0, cout_limit_NS = 0;
				int min_NS_stock = min(nbEtatStockes, NS);
				while (s < min_NS_stock)
				{
					cout_limit_NS = cout_limit_NS + 1;
					s = s + TAB_ETAT_BY_Cout[cout_limit_NS];
				}

				vector<__Etat> ETAT_SELECTIONNE_NS;
				ETAT_SELECTIONNE_NS.reserve(min_NS_stock);

				int cpt_NS = 0;
				for (int i_cmp = 0; i_cmp < nbEtat_suiv && cpt_NS < min_NS_stock; i_cmp++) //new cmp devient 10
				{
					e = l_etatsSuivant[i_cmp];
					int energ_prod = H.Energie[tour[e.i]][e.CH_h2] - e.cit - e.CH_h2; //énergie à produire
					if (energ_prod < 0)energ_prod = 0;
					if (e._cout + H.VAL[t][e.E][energ_prod] + (H.beta * H.Temps[tour[e.i]][e.CH_h2]) <= cout_limit_NS)
					{
						ETAT_SELECTIONNE_NS.push_back(e);
						cpt_NS++;
						//if (e._cout == 160)
						//	cout << "me";



					}

				}

				tab_Etat.set(tsuiv, isuiv, ETAT_SELECTIONNE_NS);
			}
			//NS procedure fin
			//std::cout << endl;
			//cout << "le nombre d'etats supprimee est:" << compt_etat_supprime;
			//std::cout << "Le nombre d'etats cree a t=" << tsuiv << ", i = " << isuiv << "est : " << nbEtat_suiv << endl;
			//std::cout << "Le nombre d'etats cree avec NS a t=" << t << "est : " << L_etat[t + 1].size() << endl << endl;
			if (l_etatsSuivant.size() > st_3)
			{
				st_3 = static_cast<int>(l_etatsSuivant.size());
				st_3_t = t;

			}
			//std::cout << endl;
			POS = POS + static_cast<int>(l_etatsSuivant.size());



		}//fin while sur i

	}//fin while t <=  Nombre_de_periode - 1

	std::cout << "le nombre d'etats cree est : " << POS << endl << endl;
}

Val_nb_State_progdyn Programdyn::printf(string num_file, Heuristique &H, double CpuT)
{
	int Min_cout = INFINI;
	__Etat Sol_final; 
	int tper=0;
	//int K_opt , T_opt;
	//parmis les états qui sont arrivés au depot final, on cherche le meilleur
	for (int t = 0; t <= H.Nombre_de_periode; t++)
	{
		vector<__Etat> l_etat = tab_Etat.get(t, H.Nb_stations + 1);
		for (int k = 0; k < l_etat.size(); k++)
		{
#ifdef VERIF_
			if (l_etat[k].i != H.Nb_stations + 1)
				stopProg("Programdyn::printf : la station devrait etre NbStation+1");
#endif
			if ((l_etat[k]._cout < Min_cout) && (l_etat[k].cit >= H.s0) && (l_etat[k].CH_h2 >= H.v0)) //!!!!!!!!!!important : val initiales dans la citerne=val finales dans la citerne 
																								   // idem pour le véhicule val initiales = valeurs finales
			{
				//K_opt = k;
				//T_opt = t;
				Sol_final = l_etat[k];
				Min_cout = l_etat[k]._cout;
				tper = t;
			}

		}
	}


	if (Min_cout == INFINI) //on n'a pas trouve de solution
	{
		Cout_prog_dy_global = -1;
		std::cout << "pas de solution (liste vide)" << endl;
		ofstream ArmonFlux_instances_excel_EXP("ArSolutionProgDyn_NS_" + num_file + ".csv");//, ifstream::app

		ArmonFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << ";" << H.p << ";" << num_file << ";" << st_3 << ";" << -1 << ";" << BSUP << ";" << -1 << ";" << st_3_t << ";" << st_3_i << ";" << H.vmax*H.smax*H.Nb_stations*H.Nombre_de_periode * 2 << ";" << H.vmax*H.smax * 2 << ";" << CpuT << endl;

		ArmonFlux_instances_excel_EXP.close();
#ifdef BSUP_PROGRAMME_DYNAMIQUE
		BSUP = -1;
		std::string cheminbsup = "cout_BSUP.txt";
		ofstream cheminbsupFlux_instances(cheminbsup, ifstream::app);
		cheminbsupFlux_instances << BSUP << endl;
#endif
#ifdef NS_PROGRAMME_DYNAMIQUE
		std::cout << "Le cout de la solution NS du programme dynamique global est :" << -1 << endl;
		std::string cheminbsup_in = "cout_BSUP_Greedy_50.txt";
		ofstream fluxcheminbsup_in("cout_BSUP_Greedy_50.txt", ifstream::app);
		fluxcheminbsup_in << -1 << endl;
#endif
	}
	else//on a trouve une solution : le dernier etat est Sol_final
	{

		//DEBUG : affichage du chemin optimal
		/*vector<__Etat> Chemin_Opt;
		vector<__Etat> l_etat = tab_Etat.get(T_opt, H.Nb_stations + 1);
		__Etat e = l_etat[K_opt];
		Chemin_Opt.push_back(e);
		while (e.i_ant != -1)
		{
			Chemin_Opt.push_back(e);
			l_etat = tab_Etat.get(e.t_ant, e.i_ant);
			e = l_etat[e.Et_ant];
		}
		Chemin_Opt.push_back(e);*/
		//DEBUG
		//on reconstruit la solution et on verifie
		//SolutionSMEPC 
		sol = construireSolution(Sol_final, H);
#ifdef VERIF_
		int duree_tour = verifrecalculedistance(sol, H); //sans attente
#endif
		

#ifdef	GENERER_INSTANCE_RESEAU_NEURONES
		int coutusi = verifrecalcule_coutusine(sol, H);
		string chemiReseauN = "InstanceReseauNeurones/RN_instance_prod__" + num_file;
		chemiReseauN = chemiReseauN + ".txt";
		ofstream monwwFlux_instances_Reseau_Neuronne(chemiReseauN, ifstream::app);
		monwwFlux_instances_Reseau_Neuronne << coutusi << endl;
		int tempRN = H.Nombre_de_periode - 1;
		while (tempRN >= 0 && !sol.periodeRecharge[tempRN])
			tempRN--;
		monwwFlux_instances_Reseau_Neuronne << tempRN * H.p << endl;
#endif
#ifdef		GENERER_INSTANCE_RESEAU_NEURONES_suite_y_m_M

		string chemiReseauN_s = "InstanceReseauNeurones_suite/RN_instance_prod_suite__" + num_file;
		chemiReseauN_s = chemiReseauN_s + ".txt";
		ofstream monwwFlux_instances_Reseau_Neuronne_s(chemiReseauN_s, ifstream::app);
		for (int t = 0; t < H.Nombre_de_periode; t++)
		{
			monwwFlux_instances_Reseau_Neuronne_s << sol.periodeProd[t] << " "; //vecteur y de RN

		}
		monwwFlux_instances_Reseau_Neuronne_s << endl;
#endif

		

		Cout_prog_dy_global = static_cast<float>(Sol_final._cout);// avec attente

		cout << "le cout de la solution est :" << Sol_final._cout;

#ifdef VERIF_
		int dureeavecattente = verifduree_total_tournee(sol, H); //avec attente


		bool verifieusine = verifieusine_ne_produit_pas_durant_recharge(sol, H);
		//bool verifqteciterne = verifrecalcule_qte_h2_dans_citerne(sol, H);
		bool verifreservoir = verifrecalcule_qte_h2_dans_reservoir(sol, H);

		//if ( (duree_tour > dureeavecattente) || (H.beta * dureeavecattente + coutusi != Sol_final._cout) ) //duree_tour
		//	stopProg("printf : le cout de la solution est faux");

		//if (!verifieusine || !verifqteciterne || !verifreservoir)
		//	stopProg("printf : solution non valide");
#endif


		__Etat etat_courant;
		//fichier excel
		std::string chemin2{ "solutions_dyn_global/solutions_ex_" };
		std::string chemin2_{ ".csv" };
		chemin2 = chemin2 + num_file + chemin2_;

		etat_courant = Sol_final;
		ofstream monFlux_instances_excel(chemin2);
		std::cout << endl;
		monFlux_instances_excel << "La solution est :" << endl;

		monFlux_instances_excel << "t" << ";" << "Citerne" << ";" << "Etat machine" << ";" << "Tournee" << ";" << "reservoir" << ";" << "cout" << ";" << "delta" << endl;
		cout << "Temps : au debut de la periode ; Etat" << endl;
		std::cout << "Temps" << ";" << "Citerne" << ";" << "Etat machine" << ";" << "Tournee" << ";" << "reservoir" << ";" << "cout" << ";" << "delta" << endl;




		while (etat_courant.t_ant != -1)
		{
			monFlux_instances_excel << tper << ";" << etat_courant.cit << ";" << etat_courant.E << ";" << tour[etat_courant.i]
				<< ";" << etat_courant.CH_h2 << ";" << etat_courant._cout << ";" << etat_courant.delta << endl;

			std::cout << tper << ";" << etat_courant.cit << ";" << etat_courant.E << ";" << tour[etat_courant.i] << ";"
				<< etat_courant.CH_h2 << ";" << etat_courant._cout << ";" << etat_courant.delta << endl;


			tper = etat_courant.t_ant;
			etat_courant = tab_Etat.get(etat_courant.t_ant, etat_courant.i_ant, etat_courant.Et_ant);

		}

		monFlux_instances_excel << tper << ";" << etat_courant.cit << ";" << etat_courant.E << ";" << tour[etat_courant.i] << ";" << etat_courant.CH_h2 << ";" << etat_courant._cout << ";" << etat_courant.delta << endl;
		std::cout << tper << ";" << etat_courant.cit << ";" << etat_courant.E << ";" << tour[etat_courant.i] << ";" << etat_courant.CH_h2 << ";" << etat_courant._cout << ";" << etat_courant.delta << endl;



		//std::cout << endl << "Le cout de la solution BUP  est :" << BSUP << endl; //cout BSUP du prog global
#ifdef PROGRAMME_DYNAMIQUE
		std::cout << "Le cout de la solution du programme dynamique global est :" << Cout_prog_dy_global << endl;
#endif
#ifdef BSUP_PROGRAMME_DYNAMIQUE
		BSUP = Cout_prog_dy_global;
		std::cout << "Le cout de la solution BSUP est :" << BSUP << endl;
		std::string cheminbsup = "cout_BSUP.txt";
		ofstream cheminbsupFlux_instances(cheminbsup, ifstream::app);
		cheminbsupFlux_instances << BSUP << endl;
#endif
#ifdef NS_PROGRAMME_DYNAMIQUE
		std::cout << "Le cout de la solution NS du programme dynamique global est :" << Cout_prog_dy_global << endl;
		std::string cheminbsup_in = "cout_BSUP_Greedy_50.txt";
		ofstream fluxcheminbsup_in("cout_BSUP_Greedy_50.txt", ifstream::app);
		fluxcheminbsup_in << Cout_prog_dy_global << endl;
#endif
		double gaap = ((BSUP - Cout_prog_dy_global) / Cout_prog_dy_global) * 100;
		std::cout << "Le gap est :" << gaap << endl;

		//Fichier qui contient les résultats de l'expérimentation du prog dyn et de sa BSUP
		//ofstream monFlux_instances_excel_EXP(experimentation, ifstream::app);
		//monFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << endl;
		//monFlux_instances_excel_EXP << "" << ";" << "" << ";" << BSUP << ";" << Cout_prog_dy_global << ";" << gaap << ";" << st_3 << ";" << st_3_t << ";" << st_3_i << ";" << H.vmax << ";" << H.smax << ";" << H.vmax*H.smax*H.Nb_stations*H.TMAX * 2 << ";" << H.vmax*H.smax * 2;
		//monFlux_instances_excel_EXP << endl;
		//monFlux_instances_excel_EXP.close();



		ofstream ArmonFlux_instances_excel_EXP("ArSolutionProgDyn_NS_" + num_file + ".csv");//, ifstream::app

		ArmonFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << ";" << H.p << ";" << num_file << ";" << st_3 << ";" << Cout_prog_dy_global << ";" << BSUP << ";" << gaap << ";" << st_3_t << ";" << st_3_i << ";" << H.vmax*H.smax*H.Nb_stations*H.TMAX * 2 << ";" << H.vmax*H.smax * 2 << ";" << CpuT << endl;

		ArmonFlux_instances_excel_EXP.close();
		/*
		//enlever le filtrage coût
		ofstream ArmonFlux_instances_excel_EXP("st2ArSolutionProgDyn.csv", ifstream::app);

		ArmonFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << ";" << num_file << ";" << st_3 << ";" << Cout_prog_dy_global << ";" << BSUP << ";" << gaap << ";" << st_3_t << ";" << H.vmax*H.smax*H.Nb_stations*H.TMAX * 2;

		ArmonFlux_instances_excel_EXP.close();

		//enlever tous les filtrages coûts
		ofstream ArmonFlux_instances_excel_EXP("st1ArSolutionProgDyn.csv", ifstream::app);

		ArmonFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << ";" << num_file << ";" << st_3 << ";" << Cout_prog_dy_global << ";" << BSUP << ";" << gaap << ";" << st_3_t << ";" << H.vmax*H.smax*H.Nb_stations*H.TMAX * 2;

		ArmonFlux_instances_excel_EXP.close();*/
		/*
		monFlux_instances_excel_EXP << H.Nb_stations << ";" << H.TMAX << endl;
		cout << st_3 << ";" << st_3_t<<endl;*/
	}

	Val_nb_State_progdyn  Sol_;
	Sol_.val = Cout_prog_dy_global;
	Sol_.Nombre_etats_maximal = st_3;
	Sol_.CPU = CpuT;
	return Sol_;
}

void Programdyn::allocationTableaux(Heuristique H) //[p OK]
{

	tour.resize(H.Nb_stations + 2);
	//la tournée initialisée
	for (int i = 0; i < H.Nb_stations + 1; i++)
		tour[i] = i;
	tour[H.Nb_stations + 1] = 0;

	Etat.resize(2 * H.Nombre_de_periode);
	//L_Etat_.resize(2 * H.TMAX, vector<int>(ETAT_MAX));// liste de tous les etats   tous les instants, L_Etat[t][k] est l'etat (selon l'ordre lexicographique)
	tab_Etat.init(H.Nb_stations + 2, H.Nombre_de_periode + 1);


}

/***************************************Verifie solution***********************************************/



SolutionSMEPC Programdyn::construireSolution(__Etat & e, Heuristique & H) // p OK
{
	__Etat e_copy = e;
	SolutionSMEPC sol(&H);
	vector<bool> isInsertStation(H.Nb_stations + 1, false);

#ifdef VERIF_
	if (e_copy.t_ant == -1)
		stopProg("Programdyn::construireSolution : on part d'un etat non final");
#endif



	//int stationFils = e_copy.i;
	int qteCiterneFils = e_copy.cit;
	int tpere = e_copy.t_ant;

	do
	{

		//#ifdef VERIF_
		//		if (e_copy.actif != 1)
		//			stopProg("Programdyn::construireSolution : on utilise un etat non actif");
		//#endif

				//pere

		e_copy = tab_Etat.get(e_copy.t_ant, e_copy.i_ant, e_copy.Et_ant);

		//detour du vehicule pour recharger
		if (!isInsertStation[e_copy.i] && e_copy.cit > qteCiterneFils) //e.i == stationFils &&
		{
			isInsertStation[e_copy.i] = true;
			sol.stationAvantRecharge[e_copy.i] = true;
		}

		//quantite rechargee et periode recharge
		if (e_copy.cit > qteCiterneFils)
		{

			sol.quantiteRecharge.push_back(e_copy.cit - qteCiterneFils);
			sol.periodeRecharge[tpere] = true; //correcte
		}

		//période de produite
		if (e_copy.cit < qteCiterneFils)
		{
			sol.periodeProd[tpere] = true; //correcte
		}


		//mise a jour des fils 
		//stationFils = e_copy.i;
		qteCiterneFils = e_copy.cit;
		tpere = e_copy.t_ant;

	} while (e_copy.t_ant != -1);
	std::reverse(sol.quantiteRecharge.begin(), sol.quantiteRecharge.end());
	return sol;
}

int Programdyn::verifrecalculedistance(SolutionSMEPC s, Heuristique & H)
{
	int duree_tour = 0; //cout (distance) tournée
	for (int i = 0; i <= H.Nb_stations; i++)
	{
		if (s.stationAvantRecharge[i])
		{
			duree_tour += H.d_i0[i] + H.d_0i[i + 1] + H.p; // + duree de l'attente pour commencer la recharge au debut d'une periode ??
		}
		else
		{
			duree_tour += H.d_i[i];
		}

	}
	//duree_tour += H.d[H.Nb_stations][0];
	std::cout << "Programdyn::verifrecalculedistance : Duree de la tournee sans attente : " << duree_tour << endl;
	return duree_tour;
}

int Programdyn::verifrecalcule_coutusine(SolutionSMEPC s, Heuristique & H)// p OK
{
	int coutusine = 0;

	if (s.periodeProd[0])
		coutusine += H.Cf;

	for (int t = 0; t < H.Nombre_de_periode; t++)
	{
		if (s.periodeProd[t])
		{
			if ((t != 0) && (!s.periodeProd[t - 1]))
			{
				coutusine += H.Cf;
			}
			coutusine += H.coutv[t]; //cout prod sur t,t+1
		}
	}
	std::cout << "Programdyn::verifrecalcule_coutusine : Cout de production h2 : " << coutusine << endl;
	return coutusine;
}


bool Programdyn::verifieusine_ne_produit_pas_durant_recharge(SolutionSMEPC s, Heuristique & H)// p OK
{
	bool boolverif = true; //boolverif=false s'il y a une erreur 
	for (int t = 0; t < H.Nombre_de_periode; t++)
	{
		if ((s.periodeProd[t]) && (s.periodeRecharge[t]))
		{
			std::cout << "Programdyn::verifieusine_ne_produit_pas_durant_recharge : Erreur: Il y a production et recharge simultanement durant la periode [" << t << "," << t + 1 << "] " << endl;
			boolverif = false;
		}
	}
	return boolverif;
}

//duree avec attente
int Programdyn::verifduree_total_tournee(SolutionSMEPC s, Heuristique &H) //p OK
{
	int duree_total_tour = 0;
	int t = H.Nombre_de_periode - 1;
	int derniere_station_recharge = Programdyn::verifderniere_station_recharge(s, H);
	while ((t >= 0) && (!s.periodeRecharge[t]))
	{
		t--;
	}


	//le vehicule quitte l'usine en t+p sauf si t=-1 (pas de recharge) auquel cas durée total = distance de l'algo crecalculedistance

	if (t != -1)
	{
		//on recharge a la periode t : on quitte l'usine a la date p*(t+1)
		duree_total_tour = H.p*(t + 1) + H.d_0i[derniere_station_recharge + 1] + H.d__[derniere_station_recharge + 1];

	}
#ifdef VERIF_
	if (duree_total_tour > H.TMAX)
		stopProg("Programdyn::verifduree_total_tournee : Erreur : La duree de la tournee depasse le TMAX");
#endif//VERIF_
	std::cout << "Programdyn::verifduree_total_tournee : Duree de la tournee avec attente : " << duree_total_tour << endl;
	return duree_total_tour;
}

int Programdyn::verifderniere_station_recharge(SolutionSMEPC s, Heuristique &H)
{
	int derniere_station = -1;
	int i = H.Nb_stations;
	while ((i >= 0) && (!s.stationAvantRecharge[i]))
		i--;
	if (i != -1)
		derniere_station = i; //derniere station après laquelle on recharge
	return derniere_station;
}

//recalcule la qantite d'h2 dans la citerne pour chaque [t,t+1].
bool Programdyn::verifrecalcule_qte_h2_dans_citerne(SolutionSMEPC s, Heuristique &H) //p OK
//qteciterne[t]=quantite d'h2 dans la citerne au debut de la periode [t,t+1]
{
	bool boolverif = true; //boolverif=false s'il y a une erreur
	vector<int> quantiteciterne;
	quantiteciterne.resize(H.Nombre_de_periode);
	quantiteciterne[0] = H.s0;
	unsigned int cmp = 0;
	int qterecharge = s.quantiteRecharge[0];
	for (int t = 1; t < H.Nombre_de_periode; t++)
	{
		quantiteciterne[t] = quantiteciterne[t - 1] + s.periodeProd[t - 1] * H.rendement[t - 1] - s.periodeRecharge[t - 1] * qterecharge;
		if (s.periodeRecharge[t - 1] && s.quantiteRecharge.size() > cmp + 1)
		{
			cmp++;
			qterecharge = s.quantiteRecharge[cmp];

		}
		if ((quantiteciterne[t] < 0) || (quantiteciterne[t] > H.smax))
		{
			std::cout << "Programdyn::verifrecalcule_qte_h2_dans_citerne : Erreur : La quantite d'hydrogene dans la citerne prend de mauvaises valeurs" << endl;
			boolverif = false;
		}
	}
	return boolverif;
}


//recalcule la quantité d'h2 dans le reservoir du vehicule
bool Programdyn::verifrecalcule_qte_h2_dans_reservoir(SolutionSMEPC s, Heuristique & H) //p OK
{
	//qte_reservoir[i] = quantite d'h2 dans le reservoir en arrivant à la station i
	bool verif_qte_reservoir = true;
	vector<int> qte_reservoir;
	qte_reservoir.resize(H.Nb_stations + 2);
	qte_reservoir[0] = H.v0;
	int cmp = 0;
	for (int i = 1; i <= H.Nb_stations + 1; i++)
	{
		if (s.stationAvantRecharge[i - 1])
		{
			qte_reservoir[i] = qte_reservoir[i - 1] - H.E_i0[i - 1] - H.E_0i[i] + s.quantiteRecharge[cmp];
			cmp++;
		}
		else
		{
			qte_reservoir[i] = qte_reservoir[i - 1] - H.E_i[i - 1];
		}
		if ((qte_reservoir[i] < 0) || (qte_reservoir[i] > H.vmax))
		{
			stopProg("Programdyn::verifrecalcule_qte_h2_dans_reservoir : Erreur : La quantite d'hydrogene dans le véhicule prend de mauvaises valeurs");
			verif_qte_reservoir = false;
		}

	}
	return verif_qte_reservoir;
}

void Programdyn::VEH_maitre_PROD_esclave_sans_NS(Heuristique& H, int num_file, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_, int BS) // p OK
{
	//int NS = 100;//new
	//if (Filtrage_Heuristique_Pipe_ == false)
	//{
	//======================================Permet de mettre le temps limit====================
	auto start = std::chrono::system_clock::now();
	//}
	//======================================
	int POS = 1; //1 etat = l'etat initial

	if (BS > 0)//H.BSUP[num_file] > 0)
	{
		BSUP = BS;//H.BSUP[num_file];// +21;
		//if (num_file == 1)BSUP += 21;
		//if (num_file == 2)BSUP += 16;
		//if (num_file == 8)BSUP += 12;
	}
	else
	{
		BSUP = 2 * BSUP_MAX;
	}
	//BSUP = 4 * BSUP_MAX;//RN
	NB_DEC = 8; // Nombre de dcisions possible

	list_NS_decroissant.clear();
	std::cout << "********deb*****" << endl;
	for (int i = 0; i < H.Nombre_de_periode; i++)
	{
		Etat[i] = 0;//si jamais on fait une liste chainee a la main
	}

	for (int t = 0; t <= H.Nombre_de_periode; t++)
	{
		//======================================Permet de mettre le temps limit====================
		if (Filtrage_Heuristique_ == false)
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
	
		for (int i = 0; i <= H.Nb_stations; i++)
		{
			//======================================Permet de mettre le temps limit====================
			if (Filtrage_Heuristique_ == false)
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

			__Etat e;
			vector<__Etat> l_etats = tab_Etat.get(t, i); //liste des etats a la date (t,i)

			//parcourt tous les etats a l "instant" (t,i)
			for (unsigned int i_cmp = 0; i_cmp < l_etats.size(); i_cmp++)
			{
				e = l_etats[i_cmp];

#ifdef VERIF_
				if (i != e.i)
					stopProg("Programdyn::VEH_maitre_PROD_esclave_sans_NS : on devrait avoir i = e.i");
#endif

				INIT_BOOLEEN(e, t, H); //, &(tour[0]), &(rendement[0]), &(d[0][0]), &(H.E[0][0])

				CONSTRUCT_DECISIONS(e, t, NB_DEC, i_cmp, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
				//cout << "pg sans NS, instance="<< num_file <<"t=" <<t<<"i="<< i<< endl;
			}
			// ----  pour faire des stats : 
			vector<__Etat> l_etatsSuivant;
			int isuiv = i + 1, tsuiv = t; //instant suivant (t,i)
			if (i == H.Nb_stations)
			{
				tsuiv = t + 1;
				isuiv = 0;
			}

			l_etatsSuivant = tab_Etat.get(tsuiv, isuiv);

			if (l_etatsSuivant.size() > st_3)
			{
				st_3 = static_cast<int>(l_etatsSuivant.size());
				st_3_t = tsuiv;
				st_3_i = isuiv;
			}
			//std::cout << endl;
			POS = POS + static_cast<int>(l_etatsSuivant.size());
			// ----  FIN pour faire des stats


		}//fin pour sur i

	}//fin pour t <=  Nombre_de_periode - 1


	std::cout << "le nombre d'etats cree est : " << POS << endl << endl;
}



void Programdyn::BSUP_VEH_maitre_PROD_esclave(Heuristique& H, int num_file, bool Filtrage_exacte_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_, bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, bool Filtrage_Heuristique_, int K_FILTRAGE_HEURIS_)
{

	int POS = 1; //1 etat = l'etat initial

	list_NS_decroissant.clear();
	std::cout << "********deb*****" << endl;

	for (int i = 0; i < H.Nombre_de_periode; i++)
	{
		Etat[i] = 0;//si jamais on fait une liste chainee a la main
	}

	for (int t = 0; t <= H.Nombre_de_periode; t++)
	{
		for (int i = 0; i <= H.Nb_stations; i++)
		{
			__Etat e;
			vector<__Etat> l_etats = tab_Etat.get(t, i); //liste des etats a la date (t,i)
			vector<__Etat>::iterator it_etatt;



			for (it_etatt = l_etats.begin(); it_etatt != l_etats.end(); it_etatt++)
			{

				e = *it_etatt;

				INIT_BOOLEEN(e, t, H);

				//explore toutes les decisions possible a partir de e et garde la meilleure
				if (CONSTRUCT_DECISIONS_BSUP(e, t, NB_DEC, 0, H, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_))
					POS++;

#ifdef VERIF_
				if (i != e.i)
					stopProg("BSUP_VEH_maitre_PROD_esclave : on devrait avoir i = e.i");
#endif
			}


		}//fin pour sur i

	}//fin pour t <=  Nombre_de_periode - 1

	std::cout << "le nombre d'etats cree est : " << POS << endl << endl;
}