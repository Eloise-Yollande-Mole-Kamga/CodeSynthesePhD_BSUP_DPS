SYSTEM     = x86-64_linux
LIBFORMAT  = static_pic

#------------------------------------------------------------
#
# When you adapt this makefile to compile your CPLEX programs
# please copy this makefile and set CPLEXDIR and CONCERTDIR to
# the directories where CPLEX and CONCERT are installed.
#
#------------------------------------------------------------

#CPLEXDIR      = /opt/ibm/ILOG/CPLEX_Studio1210/cplex
#CONCERTDIR =  /opt/ibm/ILOG/CPLEX_Studio1210/concert

# ---------------------------------------------------------------------
# Compiler selection 
# ---------------------------------------------------------------------
CC = g++ -std=c++17 
#CC = /opt/gcc-7.1.0/bin/g++
#CC = g++ -std=c++11
#CC = /usr/libexec/g++ -std=c++11
#CC = ~/gcc-5.2.0/bin/g++ -std=c++11

# ---------------------------------------------------------------------
# Compiler options 
# ---------------------------------------------------------------------


#-Wno-ignored-attributes ==> a partir de gcc 6 cplex donne plein de warning "ignored-attributes"

CCOPT = -m64 -Wall -O2 -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -DIL_STD -Wno-ignored-attributes -Wno-sign-compare
CCDEBUG = -m64 -g -fPIC -fno-strict-aliasing -fexceptions -DIL_STD -Wno-ignored-attributes -Wno-sign-compare
# -pg
# ---------------------------------------------------------------------
# Link options and libraries
# ---------------------------------------------------------------------


#CPLEXLIBDIR   = $(CPLEXDIR)/lib/$(SYSTEM)/$(LIBFORMAT)
#CONCERTLIBDIR = $(CONCERTDIR)/lib/$(SYSTEM)/$(LIBFORMAT)

#CCLNDIRS  = -L$(CPLEXLIBDIR) -L$(CONCERTLIBDIR)

#CCLNFLAGS = -lconcert -lilocplex -lcplex -lm -lpthread -ldl


#CONCERTINCDIR = $(CONCERTDIR)/include
#CPLEXINCDIR   = $(CPLEXDIR)/include

#CCFLAGS = $(CCOPT) -I$(CPLEXINCDIR) -I$(CONCERTINCDIR) 


# ---------------------------------------------------------------------
# compil
# ---------------------------------------------------------------------

#LISTE DES FICHIERS .o (chaque fichier .cpp donne un fichier .o)
OBJECTS	= main.o Heuristique_essai_main_synthese.o Source_dynGlobal_essai_main_synthese.o Algo.o Instance.o Solution.o

exec : $(OBJECTS)
	$(CC) $(CCOPT) -o exe $(OBJECTS)
	

.cpp.o :
	$(CC) $(CCOPT) -c $< -o $@

clean : 
	rm -f $(OBJECTS) exe *~


#include makefile.dep
