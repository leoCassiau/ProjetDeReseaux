#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


typedef enum coup {
    pierre,
    feuille,
    ciseaux
}coup;

typedef struct  {
    char nom[256]; // pseudo
    coup c;     // coup joué
    int score;  // score actuel du joueur
    int thread; //id du thread du client
	bool isAlive; //
	int rank; //Position du joueur dans la "ronde", premier arrivé premier servi, maj par les morts et deconnexions
}Joueur;

bool bat(coup c1, coup c2) {
    if( ((c1 == pierre) && (c2 == ciseaux)) ||
        ((c1 == ciseaux) && (c2 == feuille)) ||
        ((c1 == feuille) && (c2 == pierre))) {
        return true;
    }
    return false;
}

bool attaque(Joueur j1, Joueur j2) {
    if(bat(j1.c, j2.c)) {
        return true;
    }else
    return false;
}

bool defend(Joueur j1, Joueur j2) {
    if(bat(j2.c, j1.c)) {
        return true;
    }
    return false;
}

typedef enum Operation{
	play,
	report
}Operation;


struct socketMain{
	Operation op;
	Joueur player[];
	
};

