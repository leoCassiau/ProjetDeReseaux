/* shifumi.h
 *
 * Auteurs : Léo Cassiau, Jean-Christophe Guérin
 * Mars 2016
 *
 * Faculté des sciences et techniques - Master ALMA
 * Projet de Réseaux encadré par M. Passard et Mme Hamma
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NB_MAX_JOUEURS 256
#define TAILLE_MAX_NOM 256

typedef enum {
	pierre, feuille, ciseaux, rien
} coup;

typedef enum {
	nouvellePartie, finPartie, debutTour, finTour, enAttente, erreur
} etatPartie;

typedef struct {
	char nom[TAILLE_MAX_NOM];	// Pseudonyme du joueur
	int socket; // ID du socket du client
	int rang;	// Position du joueur dans la "ronde"
	int score;  // Score actuel du joueur
	coup coup;	// Coup joué actuellement
	bool enVie; // Indique si le joueur est encore dans la partie
	bool absent;	// Indique si le joueur ne réponds plus
} Joueur;

typedef struct {
	Joueur joueur;
	bool partiePleine;
	etatPartie etat;
	Joueur joueurs[NB_MAX_JOUEURS];
	int nbJoueurs;
} Datagramme;

bool bat(coup c1, coup c2) {
	if (((c1 == pierre) && (c2 == ciseaux))
			|| ((c1 == ciseaux) && (c2 == feuille))
			|| ((c1 == feuille) && (c2 == pierre))) {
		return true;
	}
	return false;
}

bool attaque(Joueur * j1, Joueur * j2) {
	if (bat(j1->coup, j2->coup)) {
		j2->enVie = false;
		return true;
	} else
		return false;
}

const char* coupToString(coup c) {
	if(c==pierre) { return "pierre ";}
	else if (c == feuille) { return "feuille";}
	else if (c == ciseaux) { return "ciseaux";}
	else if (c == rien) { return "rien";}
	else { return "COUP_ERROR";}
}

void afficheJoueurs(Joueur joueurs[NB_MAX_JOUEURS], int nbJoueurs) {
	printf("| RANG | PSEUDO | COUP | SCORE |\n");
	printf("|------|--------|------|-------|\n");
	int i;
	for (i = 0; i < nbJoueurs; i++) {
		printf("|%d|%s|%s|%d|\n", i, joueurs[i].nom,
				coupToString(joueurs[i].coup), joueurs[i].score);
	}
	printf("|______|________|______|_______|\n");
}
