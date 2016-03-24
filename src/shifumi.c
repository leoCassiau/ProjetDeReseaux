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

// Paramètres à modifier selon les envies
#define NB_MAX_JOUEURS 256 // Nombre maximum de joueurs dans la partie
#define TAILLE_MAX_NOM 256 // Taille maximum du pseudonyme

// Enumère les différents coups possibles au shifumi
typedef enum {
	pierre, feuille, ciseaux, rien
} coup;

// Enumère les différents états que peut avoir la partie
typedef enum {
    nouvellePartie, finPartie, debutTour, finTour, enAttente, erreur, attendsFinTour
} etatPartie;

// Structure contenant les différentes informations d'un joueur
typedef struct {
	char nom[TAILLE_MAX_NOM];	// Pseudonyme du joueur
	int socket; // ID du socket du client
	int rang;	// Position du joueur dans la "ronde"
	int score;  // Score actuel du joueur
	coup coup;	// Coup joué actuellement
	bool enVie; // Indique si le joueur est encore dans la partie
	bool absent;	// Indique si le joueur ne réponds plus
} Joueur;

// Structure contenant les données envoyés entre le client et le serveur
typedef struct {
	Joueur joueur; // Données d'un joueur particulier (le gagnant de la partie par exemple)
	bool partiePleine; // Indique si la partie est pleine ou non
	etatPartie etat; // Donne l'état actuel de la partie
	Joueur joueurs[NB_MAX_JOUEURS]; // Tableau de joueurs contenant tous les joueurs actuels de la partie
	int nbJoueurs; // Taille du tableau joueurs[]
} Datagramme;

/**
 * @brief Joue le coup c1 contre le coup c2 au shifumi
 * @param c1 Coup de l'attaquant
 * @param c2 Coup du défenseur
 * @return Vrai si c1 bat c2, faux sinon
 */
bool bat(coup c1, coup c2) {
	if (((c1 == pierre) && (c2 == ciseaux))
			|| ((c1 == ciseaux) && (c2 == feuille))
			|| ((c1 == feuille) && (c2 == pierre))
			|| (c2==rien) ) {
		return true;
	}
	return false;
}

/**
 * @brief Le joueur j1 attaque j2 au shifumi
 * @param j1 Joueur attaquant
 * @param j2 Joueur defenseur
 * @return Vrai si j1 bat j2, faux sinon
 */
bool attaque(Joueur * j1, Joueur * j2) {
	if (bat(j1->coup, j2->coup)) {
		j2->enVie = false;
		return true;
	} else
		return false;
}

/**
 * @brief Transforme un coup en chaîne de caractère correspondante (Feuille, Ciseaux, Pierre ou Rien)
 * @param c Coup transformé en chaîne de caractère
 * @return Chaîne de caractère représentant c
 */
const char* coupToString(coup c) {
	if(c==pierre) { return "Pierre ";}
	else if (c == feuille) { return "Feuille";}
	else if (c == ciseaux) { return "Ciseaux";}
    else if (c == rien) { return "Rien   ";}
	else { return "COUP_ERROR";}
}

/**
 * @brief Affiche un tableau de joueurs et leurs informations essentiels
 * @param joueurs Tableau de joueur à afficher
 * @param nbJoueurs Taille de joueurs[]
 */
void afficheJoueurs(Joueur joueurs[NB_MAX_JOUEURS], int nbJoueurs) {
    printf(" _______________________________\n");
	printf("| RANG | SCORE | COUP  | PSEUDO \n");
	printf("|------|-------|-------|--------\n");
	int i;
	for (i = 0; i < nbJoueurs; i++) {
		printf("|     %d|      %d|%s|%s\n", i, joueurs[i].score,
				coupToString(joueurs[i].coup), joueurs[i].nom);
	}
	printf("|______|_______|_______|________\n");
}
