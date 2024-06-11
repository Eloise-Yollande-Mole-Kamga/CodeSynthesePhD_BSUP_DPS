//ALGORITHME QUI CHERCHE AVEC LE DPS LE COUT DE LA MEILLEURE SOLUTION
//EN ESSAYANT D ETRE LE RAPIDE
#include "Header_essai_main_synthese.h"
#include "Algo.h"
//========================================================================
//Programme dynamique
#include <iostream>
#include<stdio.h>
#include<math.h>
#include<time.h>
#include <string>
#include<algorithm>
#include<fstream>
#include<chrono>
#include<list>

//Les solutions sont SolBsup_Helene, SolPipeline, SolProgDynGlobal, SolProgDynNS[3]

using namespace std;



int main(int argc, char **argv)
{


	srand(static_cast<unsigned int>(time(NULL)));
	int iMax=0;// = 8;//60
	int iMin=0;// = 2;

	if (argc == 2)
	{
		iMin = atoi(argv[1]);
		iMax = iMin;
		cout << "numero du fichier a lire : " << iMin << endl;
	}
	std::string chemin__{ ".csv" };
	std::string chemin = "Synthese" +to_string(iMin)+ chemin__; //+to_string(i)
	ofstream ArmonFlux_instances(chemin);//, ifstream::app);
	/*ArmonFlux_instances << "Num Instance" << ";" << "# stations" << ";" << "# périodes" << ";" << "p" << ";" << "Val BSUP He" << ";" << "CPU BSUP He" << ";"
		<< "Val NS 20" << ";" << "CPU NS 20" << ";" << "Val NS 50" << ";" << "CPU NS 50" << ";" << "Val NS 100" << ";" << "CPU NS 100" << ";"
		<< "Val Pipeline(3) " << ";" << "# etats recharge Pipeline(3)" << ";" << "# etats prod Pipeline(3)" << ";" << "CPU pipeline(3)" << ";"
		<< "Val Pipeline(2) " << ";" << "# etats recharge Pipeline(2)" << ";" << "# etats prod Pipeline(2)" << ";" << "CPU pipeline(2)" << ";"
		<< "Val Pipeline(2) " << ";" << "# etats recharge Pipeline(1)" << ";" << "# etats prod Pipeline(1)" << ";" << "CPU pipeline(1)" << ";"
		<< "Val Pipeline(0) " << ";" << "# etats recharge Pipeline(0)" << ";" << "# etats prod Pipeline(0)" << ";" << "CPU pipeline(0)" << ";"
		<< "Val St(3)" << ";" << "# etats St(3)" << ";" << "CPU St(3)" << ";"
		<< "Val St(2)" << ";" << "# etats St(2)" << ";" << "CPU St(2)" << ";"
		<< "Val St(1)" << ";" << "# etats St(1)" << ";" << "CPU St(1)" << ";"
		<< "Val St(0)" << ";" << "# etats St(0)" << ";" << "CPU St(0)" << ";" << endl;
	*/

	for (int i = iMin; i <= iMax; i++)
	{
		string  num_file = to_string(i);
		Heuristique test;


		//===================================================================================Instance===============================================================//

	
		/*string fichier_production = "instances_avec_p/instance_Prod__"; 
		string fichier_instance = "instances_avec_p/instance__";  //*/

		string fichier_production = "datasetAlejandro_1_19_datasetRAIRO_20_34/instance_Prod__"; 
		string fichier_instance = "datasetAlejandro_1_19_datasetRAIRO_20_34/instance__";  //

		//===========================poub
		/*string fichier_production = "15_Instances/instance_Prod__";
		string fichier_instance = "15_Instances/instance__";*/

		/*string fichier_production = "instances_avec_p_PL/instance_Prod__"; //"15_Instances/instance_Prod__";
		string fichier_instance = "instances_avec_p_PL/instance__";  //"15_Instances/instance__";*/

		/*string fichier_production = "NNdatasetHelene/instance_Prod__"; //"15_Instances/instance_Prod__";
		string fichier_instance = "NNdatasetHelene/instance__";  //"15_Instances/instance__";*/
		//===========================

		//===================================================================================Heuristique Rapide Helene===============================================================//


		auto start = std::chrono::system_clock::now();
		//int BS= calcul_BS_heuristique("instances_avec_p/instance__" + num_file + ".txt", "instances_avec_p/instance_Prod__" + num_file + ".txt");
		//int BS = calcul_BS_heuristique("NNdatasetHelene/instance__" + num_file + ".txt", "NNdatasetHelene/instance_Prod__" + num_file + ".txt");
		int BS = calcul_BS_heuristique(fichier_instance + num_file + ".txt", fichier_production + num_file + ".txt");
		auto end = std::chrono::system_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		double CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_Cpu_Bsup SolBsup_Helene;
		SolBsup_Helene.val = BS;
		SolBsup_Helene.CPU = CpuT;
		test.read_instance_in_file(fichier_instance, num_file, fichier_production);
		/******Recharge*********/
		test.Tri_bulle();
		test.Estimation_energie_temps();
		test.dyn_Recharge_temps();
		test.dyn_Recharge_energie();
		/***********Production******/
		test.Cout_min_prod();
		

		//===================================================================================PROGRAMME DYNAMIQUE NS===============================================================//
		bool Filtrage_exacte_;
		bool PROGRAMME_DYNAMIQUE_SANS_filtrage_;
		bool PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_;
		bool Filtrage_Heuristique_;
		int K_FILTRAGE_HEURIS_;
		
		vector<int> vNS = { 20, 50, 100 };
		Filtrage_exacte_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PROGRAMME_DYNAMIQUE_SANS_filtrage_ = false; ////On active ceci (true) si on veut enlever le filtrage
		PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_ = false;//On active ceci si on veut enlever le filtrage bSUP
		Filtrage_Heuristique_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
		K_FILTRAGE_HEURIS_ = 7;
		Val_nb_State_progdyn SolProgDynNS[3];//Nombre de NS
		int it_SolProgDynNS = 0;
		//1. on essaie d'ameliorer la BSUP (avec des algo rapides -> filtrage sur le nb d'etats)
		for (int i_NS = 0; i_NS < vNS.size(); ++i_NS)
		{
			//on cherche une solution strictement meilleure que BS
			//Global vehicle/production
			Programdyn ProgDynNS;
			ProgDynNS.allocationTableaux(test);
			ProgDynNS.GENERER_ETAT_INIT(test);
			start = std::chrono::system_clock::now();// vNS[i_NS] = 50; BS = 145;
			ProgDynNS.VEH_maitre_PROD_esclave(test, i, vNS[i_NS], Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
			//affichage du temps d'exécution
			end = std::chrono::system_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			std::cout << elapsed.count() << '\n';
			CpuT = static_cast<double> (elapsed.count()) / 1000;
			SolProgDynNS[it_SolProgDynNS] = ProgDynNS.printf(num_file, test, CpuT);
			if (SolProgDynNS[it_SolProgDynNS].val > 0 && SolProgDynNS[it_SolProgDynNS].val < BS)//on a trouve une solution : elle est forcement meilleure
				BS = SolProgDynNS[it_SolProgDynNS].val;
			if (SolProgDynNS[it_SolProgDynNS].val == -1)
				SolProgDynNS[it_SolProgDynNS].val = BS;
			it_SolProgDynNS++;
		}
		

		//===================================================================================PIPELINE===============================================================//

		bool AlgoOk;
		/****************Pipeline vehicle/production*************///========Pi(3)
		/*Heuristique test_pi_3;
		test_pi_3.read_instance_in_file(fichier_instance, num_file, fichier_production);
		//Recharge
		test_pi_3.Tri_bulle();
		test_pi_3.Estimation_energie_temps();
		test_pi_3.dyn_Recharge_temps();
		test_pi_3.dyn_Recharge_energie();
		//Production
		test_pi_3.Cout_min_prod();
		start = std::chrono::system_clock::now();
		bool Filtrage_exacte_pipe_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		bool PIPELINE_SANS_filtrage_BSUP_ = false;
		bool PIPELINE_SANS_filtrage_ = false; //On active ceci si on veut enlever le filtrage
		bool Filtrage_Heuristique_Pipe_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 09/05/2022
		int K_FILTRAGE_HEURIS_Pipe_ = 3;
		//Recharge
		int alphaa = 1; //energie
		int betaa = test_pi_3.beta;//temps
		test_pi_3.dyn_Recharge(alphaa, betaa);
		test_pi_3.intervalles_recharge();
		test_pi_3.extraction_tour(num_file, alphaa, betaa);
		//Production
		int lambdaa = betaa * test_pi_3.p;
		//BS = BS ;
		int BS_pi_3 = BS;
		AlgoOk = test_pi_3.dyn_Production_flexible(i, lambdaa, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		//affichage du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_pipeline SolPipeline;//Pipeliene avec tous les filtrages exactes
		if (AlgoOk || (CpuT <= TEMPS_LIMIT))
		{
			SolPipeline = test_pi_3.Pipeline_T_Reconstruction_procedure(num_file, CpuT, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		}
		else
		{
			SolPipeline.CPU = CpuT;
			SolPipeline.Nombre_etats_maximal_recharge = test_pi_3.Nombre_etats_maximal_recharge;
			SolPipeline.Nombre_etats_maximal_prod = test_pi_3.Nombre_etats_maximal_prod;
			SolPipeline.val = 0;
		}
		if (SolPipeline.val > 0 && SolPipeline.val < BS)//on a trouve une solution : elle est forcement meilleure
			BS = SolPipeline.val;
		if (SolPipeline.val == -1)
			SolPipeline.val = BS;


		/****************Pipeline vehicle/production*************///========Pi(2)
		/*Heuristique test_pi_2;
		test_pi_2.read_instance_in_file(fichier_instance, num_file, fichier_production);
		//Recharge
		test_pi_2.Tri_bulle();
		test_pi_2.Estimation_energie_temps();
		test_pi_2.dyn_Recharge_temps();
		test_pi_2.dyn_Recharge_energie();
		//Production
		test_pi_2.Cout_min_prod();
		start = std::chrono::system_clock::now();
		Filtrage_exacte_pipe_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PIPELINE_SANS_filtrage_BSUP_ = true;
		PIPELINE_SANS_filtrage_ = false; //On active ceci si on veut enlever le filtrage
		Filtrage_Heuristique_Pipe_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 09/05/2022
		K_FILTRAGE_HEURIS_Pipe_ = 3;
		//Recharge
		alphaa = 1; //energie
		betaa = test_pi_2.beta;//temps
		test_pi_2.dyn_Recharge(alphaa, betaa);
		test_pi_2.intervalles_recharge();
		test_pi_2.extraction_tour(num_file, alphaa, betaa);
		//Production
		lambdaa = betaa * test_pi_2.p;
		AlgoOk = test_pi_2.dyn_Production_flexible(i, lambdaa, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		//Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		//auto elapsed = end - start;
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_pipeline SolPipeline_pi_2;
		if (AlgoOk || (CpuT<= TEMPS_LIMIT))
		{
			SolPipeline_pi_2 = test_pi_2.Pipeline_T_Reconstruction_procedure(num_file, CpuT, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		}
		else
		{
			SolPipeline_pi_2.CPU = CpuT;
			SolPipeline_pi_2.Nombre_etats_maximal_recharge = test_pi_2.Nombre_etats_maximal_recharge;
			SolPipeline_pi_2.Nombre_etats_maximal_prod = test_pi_2.Nombre_etats_maximal_prod;
			SolPipeline_pi_2.val = 0;
		}
		if (SolPipeline_pi_2.val > 0 && SolPipeline_pi_2.val < BS)//on a trouve une solution : elle est forcement meilleure
			BS = SolPipeline_pi_2.val;
		//if (SolPipeline_pi_2.val == -1)
			//SolPipeline_pi_2.val = BS;


		/****************Pipeline vehicle/production*************///========Pi(1)
		/*Heuristique test_pi_1;
		test_pi_1.read_instance_in_file(fichier_instance, num_file, fichier_production);
		//Recharge
		test_pi_1.Tri_bulle();
		test_pi_1.Estimation_energie_temps();
		test_pi_1.dyn_Recharge_temps();
		test_pi_1.dyn_Recharge_energie();
		//Production
		test_pi_1.Cout_min_prod();
		start = std::chrono::system_clock::now();
		Filtrage_exacte_pipe_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PIPELINE_SANS_filtrage_BSUP_ = false;
		PIPELINE_SANS_filtrage_ = true; //On active ceci si on veut enlever le filtrage
		Filtrage_Heuristique_Pipe_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 09/05/2022
		K_FILTRAGE_HEURIS_Pipe_ = 3;
		//Recharge
		alphaa = 1; //energie
		betaa = test_pi_1.beta;//temps
		test_pi_1.dyn_Recharge(alphaa, betaa);
		test_pi_1.intervalles_recharge();
		test_pi_1.extraction_tour(num_file, alphaa, betaa);
		//Production
		lambdaa = betaa * test_pi_1.p;
		AlgoOk = test_pi_1.dyn_Production_flexible(i, lambdaa, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		//Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_pipeline SolPipeline_pi_1;
		if (AlgoOk || (CpuT <= TEMPS_LIMIT))
		{
			SolPipeline_pi_1 = test_pi_1.Pipeline_T_Reconstruction_procedure(num_file, CpuT, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		}
		else
		{
			SolPipeline_pi_1.CPU = CpuT;
			SolPipeline_pi_1.Nombre_etats_maximal_recharge = test_pi_1.Nombre_etats_maximal_recharge;
			SolPipeline_pi_1.Nombre_etats_maximal_prod = test_pi_1.Nombre_etats_maximal_prod;
			SolPipeline_pi_1.val = 0;
		}
		if (SolPipeline_pi_1.val > 0 && SolPipeline_pi_1.val < BS)//on a trouve une solution : elle est forcement meilleure
			BS = SolPipeline_pi_1.val;
		//if (SolPipeline_pi_1.val == -1)
			//SolPipeline_pi_1.val = BS;


		/****************Pipeline vehicle/production*************///========Pi(0)
		/*Heuristique test_pi_0;
		test_pi_0.read_instance_in_file(fichier_instance, num_file, fichier_production);
		//Recharge
		test_pi_0.Tri_bulle();
		test_pi_0.Estimation_energie_temps();
		test_pi_0.dyn_Recharge_temps();
		test_pi_0.dyn_Recharge_energie();
		//Production
		test_pi_0.Cout_min_prod();
		start = std::chrono::system_clock::now();
		Filtrage_exacte_pipe_ = false; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PIPELINE_SANS_filtrage_BSUP_ = false;
		PIPELINE_SANS_filtrage_ = false; //On active ceci si on veut enlever le filtrage
		Filtrage_Heuristique_Pipe_ = true;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 09/05/2022
		K_FILTRAGE_HEURIS_Pipe_ = 3;
		//Recharge
		alphaa = 1; //energie
		betaa = test_pi_0.beta;//temps
		test_pi_0.dyn_Recharge(alphaa, betaa);
		test_pi_0.intervalles_recharge();
		test_pi_0.extraction_tour(num_file, alphaa, betaa);
		//Production
		lambdaa = betaa * test_pi_0.p;
		AlgoOk = test_pi_0.dyn_Production_flexible(i, lambdaa, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		//Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_pipeline SolPipeline_pi_0;
		//if (AlgoOk)
		//{
		SolPipeline_pi_0 = test_pi_0.Pipeline_T_Reconstruction_procedure(num_file, CpuT, Filtrage_exacte_pipe_, PIPELINE_SANS_filtrage_BSUP_, PIPELINE_SANS_filtrage_, Filtrage_Heuristique_Pipe_, K_FILTRAGE_HEURIS_Pipe_, BS);
		//}
		if (SolPipeline_pi_0.val > 0 && SolPipeline_pi_0.val < BS)//on a trouve une solution : elle est forcement meilleure
			BS = SolPipeline_pi_0.val;
		if (SolPipeline_pi_0.val == -1)
			SolPipeline_pi_0.val = BS;


		//===================================================================================PROGRAMME DYNAMIQUE GLOBAL===============================================================//
		

		/****************Global vehicle/production*************///========st(2)
		Programdyn ProgDynGlobal_st_2;
		ProgDynGlobal_st_2.allocationTableaux(test);
		ProgDynGlobal_st_2.GENERER_ETAT_INIT(test);
		start = std::chrono::system_clock::now();
		Filtrage_exacte_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PROGRAMME_DYNAMIQUE_SANS_filtrage_ = false; ////On active ceci (true) si on veut enlever le filtrage
		PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_ = true;//On active ceci si on veut enlever le filtrage bSUP
		Filtrage_Heuristique_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
		K_FILTRAGE_HEURIS_ = 7;
		ProgDynGlobal_st_2.VEH_maitre_PROD_esclave_sans_NS(test, i, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		//Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_progdyn SolProgDynGlobal_st_2;
		if (CpuT <= TEMPS_LIMIT)
		{
			SolProgDynGlobal_st_2 = ProgDynGlobal_st_2.printf(num_file, test, CpuT);
		}
		else
		{
			SolProgDynGlobal_st_2.CPU = CpuT;
			SolProgDynGlobal_st_2.Nombre_etats_maximal = ProgDynGlobal_st_2.st_3;
			SolProgDynGlobal_st_2.val = 0;
		}
		//if (SolProgDynGlobal_st_2.val == -1)
			//SolProgDynGlobal_st_2.val = BS;


		/****************Global vehicle/production*************///========st(3)
		Programdyn ProgDynGlobal_st_3;
		ProgDynGlobal_st_3.allocationTableaux(test);
		ProgDynGlobal_st_3.GENERER_ETAT_INIT(test);
		start = std::chrono::system_clock::now();
		Filtrage_exacte_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PROGRAMME_DYNAMIQUE_SANS_filtrage_ = false; ////On active ceci (true) si on veut enlever le filtrage
		PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_ = false;//On active ceci si on veut enlever le filtrage bSUP
		Filtrage_Heuristique_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
		K_FILTRAGE_HEURIS_ = 7;
		//BS = 122;
		ProgDynGlobal_st_3.VEH_maitre_PROD_esclave_sans_NS(test, i, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		//===========================Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		//auto elapsed = end - start;
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_progdyn SolProgDynGlobal_st_3;
		if (CpuT <= TEMPS_LIMIT)
		{
			SolProgDynGlobal_st_3 = ProgDynGlobal_st_3.printf(num_file, test, CpuT);
		}
		else
		{
			SolProgDynGlobal_st_3.CPU = CpuT;
			SolProgDynGlobal_st_3.Nombre_etats_maximal = ProgDynGlobal_st_3.st_3;
			SolProgDynGlobal_st_3.val = 0;
		}
		//if (SolProgDynGlobal_st_3.val == -1)
			//SolProgDynGlobal_st_3.val = BS;



		/****************Global vehicle/production*************///========st(1)
		Programdyn ProgDynGlobal_st_1;
		ProgDynGlobal_st_1.allocationTableaux(test);
		ProgDynGlobal_st_1.GENERER_ETAT_INIT(test);
		start = std::chrono::system_clock::now();
		Filtrage_exacte_ = true; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PROGRAMME_DYNAMIQUE_SANS_filtrage_ = true; ////On active ceci (true) si on veut enlever le filtrage
		PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_ = false;//On active ceci si on veut enlever le filtrage bSUP
		Filtrage_Heuristique_ = false;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
		K_FILTRAGE_HEURIS_ = 7;
		ProgDynGlobal_st_1.VEH_maitre_PROD_esclave_sans_NS(test, i, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		//Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_progdyn SolProgDynGlobal_st_1;
		if (CpuT <= TEMPS_LIMIT)
		{
			SolProgDynGlobal_st_1 = ProgDynGlobal_st_1.printf(num_file, test, CpuT);
		}
		else
		{
			SolProgDynGlobal_st_1.CPU = CpuT;
			SolProgDynGlobal_st_1.Nombre_etats_maximal = ProgDynGlobal_st_1.st_3;
			SolProgDynGlobal_st_1.val = 0;
		}
		//if (SolProgDynGlobal_st_1.val == -1)
			//SolProgDynGlobal_st_1.val = BS;



		/****************Global vehicle/production*************///========st(0)
		Programdyn ProgDynGlobal_st_0;
		ProgDynGlobal_st_0.allocationTableaux(test);
		ProgDynGlobal_st_0.GENERER_ETAT_INIT(test);
		start = std::chrono::system_clock::now();
		Filtrage_exacte_ = false; //Toujours ajouter ceci si on veut faire du filtrage exacte ou si on ne veut pas faire de filtrage. Par contre si on veut faire du filtrage heuristique on desactive ceci
		PROGRAMME_DYNAMIQUE_SANS_filtrage_ = false; ////On active ceci (true) si on veut enlever le filtrage
		PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_ = false;//On active ceci si on veut enlever le filtrage bSUP
		Filtrage_Heuristique_ = true;//On active ceci si on veut ajouter le filtrage heuristique !!!ajouter le 06/05/2022
		K_FILTRAGE_HEURIS_ = 7;
		ProgDynGlobal_st_0.VEH_maitre_PROD_esclave_sans_NS(test, i, Filtrage_exacte_, PROGRAMME_DYNAMIQUE_SANS_filtrage_, PROGRAMME_DYNAMIQUE_SANS_filtrage_BSUP_, Filtrage_Heuristique_, K_FILTRAGE_HEURIS_, BS);
		//===========================Enregistrement du temps d'exécution
		end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << elapsed.count() << '\n';
		CpuT = static_cast<double> (elapsed.count()) / 1000;
		Val_nb_State_progdyn SolProgDynGlobal_st_0;
		SolProgDynGlobal_st_0 = ProgDynGlobal_st_0.printf(num_file, test, CpuT);
		if (SolProgDynGlobal_st_0.val == -1)
			SolProgDynGlobal_st_0.val = BS;


		//===================================================================================COnstruction du fichier de tous les résultats===============================================================//


		ArmonFlux_instances << num_file << ";" << test.Nb_stations << ";" << test.Nombre_de_periode << ";" << test.p << ";" << SolBsup_Helene.val << ";" << SolBsup_Helene.CPU << ";"
			<< SolProgDynNS[0].val << ";" << SolProgDynNS[0].CPU << ";" << SolProgDynNS[1].val << ";" << SolProgDynNS[1].CPU << ";" << SolProgDynNS[2].val << ";" << SolProgDynNS[2].CPU << ";"
			//<<BS_pi_3 << ";" << SolPipeline.val << ";" << SolPipeline.Nombre_etats_maximal_recharge << ";" << SolPipeline.Nombre_etats_maximal_prod << ";" << SolPipeline.CPU << ";"
			//<< SolPipeline_pi_2.val << ";" << SolPipeline_pi_2.Nombre_etats_maximal_recharge << ";" << SolPipeline_pi_2.Nombre_etats_maximal_prod << ";" << SolPipeline_pi_2.CPU << ";"
			//<< SolPipeline_pi_1.val << ";" << SolPipeline_pi_1.Nombre_etats_maximal_recharge << ";" << SolPipeline_pi_1.Nombre_etats_maximal_prod << ";" << SolPipeline_pi_1.CPU << ";"
			//<< SolPipeline_pi_0.val << ";" << SolPipeline_pi_0.Nombre_etats_maximal_recharge << ";" << SolPipeline_pi_0.Nombre_etats_maximal_prod << ";" << SolPipeline_pi_0.CPU << ";"
			<< SolProgDynGlobal_st_3.val << ";" << SolProgDynGlobal_st_3.Nombre_etats_maximal << ";" << SolProgDynGlobal_st_3.CPU << ";"
			<< SolProgDynGlobal_st_2.val << ";" << SolProgDynGlobal_st_2.Nombre_etats_maximal << ";" << SolProgDynGlobal_st_2.CPU << ";"
			<< SolProgDynGlobal_st_1.val << ";" << SolProgDynGlobal_st_1.Nombre_etats_maximal << ";" << SolProgDynGlobal_st_1.CPU << ";"
			<< SolProgDynGlobal_st_0.val << ";" << SolProgDynGlobal_st_0.Nombre_etats_maximal << ";" << SolProgDynGlobal_st_0.CPU << ";" 
			<< endl;


	}
	ArmonFlux_instances.close();
	//system("pause");
	return 0;

}
