#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NB_MAX_JOUEURS 256
#define TAILLE_MAX_NOM 256

typedef enum coup {
    pierre,
    feuille,
    ciseaux,
		rien
}coup;

typedef enum etatPartie {
	debut,
	fin,
	enCours,
	enAttente
}etatPartie;

typedef struct {
    char nom[TAILLE_MAX_NOM]; // pseudo
    coup coup;     // coup joué
    int score;  // score actuel du joueur
    int thread; //id du thread du client
	bool enVie; //
	bool aJoue;
	int rank; //Position du joueur dans la "ronde", premier arrivé premier servi, maj par les morts et deconnexions
int ttl; //Durée de vie du joueur
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
    if(bat(j1.coup, j2.coup)) {
        return true;
    }else
    return false;
}

void afficheJoueurs(Joueur joueurs[NB_MAX_JOUEURS]) {
	printf("| Joueur | Coup | Score |\n");
	printf("|--------|------|-------|\n");
	int i;	
	for(i = 0; i < 256 ; i++) {
		char const * coups[3] = {"pierre","feuille","ciseaux"};
		printf("|%s|%s|%d|\n",joueurs[i].nom,coups[joueurs[i].coup],joueurs[i].score);
	}
	printf("|________|______|_______|\n");
}

typedef enum Operation{
	nouveauJoueur,
}Operation;


struct socketMain{
	Operation op;
	Joueur player[];
	
};

// AMOI
typedef struct {
	Operation operation;
	Joueur joueur;
	bool partiePleine;
} Datagramme;

