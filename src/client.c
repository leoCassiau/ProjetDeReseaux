/* client.c
 * Client du jeu Shifumi a lancer apres le serveur avec la commande :
 * 	client <adresse-serveur> <message-a-transmettre>
 *
 * Auteurs : Léo Cassiau, Jean-Christophe Guérin
 * Mars 2016
 *
 * Faculté des sciences et techniques - Master ALMA
 * Projet de Réseaux encadré par M. Passard et Mme Hamma
 */

#include "shifumi.c"
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>	// Parallelisation
#include <time.h>
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

// Variables globales pour la connexion avec le serveur
int socket_descriptor; /* descripteur de socket */
sockaddr_in adresse_locale; /* adresse de socket local */
hostent * ptr_host; /* info sur une machine hote */
servent * ptr_service; /* info sur service */
char * host; /* adresse de l'hôte*/

// Variables globales pour le déroulement du jeu
int rangClient;	// Rang du client actuel
bool joue; // Indique si le joueur à joué durant le tour actuel.

/**
 * @brief Envoie un datagramme au serveur.
 * @param data Datagramme à envoyer au serveur
 */
void sendDatagramme(Datagramme data) {
	/* envoi du message vers le serveur */
	int longueur = send(socket_descriptor, &data, sizeof(Datagramme), 0);
	// Gestion des erreurs de communication avec le serveur
	if ((longueur < 0)) {
		printf("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
	} else if ((longueur == 0)) {
		printf("erreur : La socket d'envoi a ete fermee.");
		exit(1);
	}
}

/**
 * @brief Attends la reception d'un datagramme envoyé par le serveur.
 * Fonction bloquante.
 * @return Le datagramme envoyé par le serveur.
 */
Datagramme readDatagramme() {
	/* reception du message du serveur */
	Datagramme data;
	int longueur = recv(socket_descriptor, &data, sizeof(Datagramme),
			MSG_WAITALL);
	// Gestion des erreurs de communication avec le serveur
	if ((longueur < 0)) {
		printf("erreur : impossible de lire le message recu.");
		exit(1);
	} else if ((longueur == 0)) {
		printf("erreur : La socket de reception a ete fermee.");
		exit(1);
	}

	return data;
}

/**
 * @brief Envoie au serveur un datagramme signalant la connexion d'un nouveau joueur
 * @param joueur Le nouveau joueur qui rejoint la partie de shifumi
 * @return Faux si la partie est pleine, vrai sinon.
 */
bool sendNouveauJoueur(Joueur joueur) {

	// Envoie du joueur au serveur
	Datagramme data;
	data.joueur = joueur;

	/* tentative de connexion au serveur dont les infos sont dans adresse_locale */
	if ((connect(socket_descriptor, (sockaddr*) (&adresse_locale),
			sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.");
		exit(1);
	}
	sendDatagramme(data);

	// Recupération des informations du joueur
	data = readDatagramme();
	rangClient = data.joueur.rang;

	return data.partiePleine;

}

/**
 * @brief Créer un nouveau joueur à partir de son pseudo.
 * Initialise un joueur avec le pseudo donné, un score à 0, qui n'est pas "en vie", qui n'a pas de coup et qui n'est pas absent.
 * @param pseudo Pseudonyme du joueur
 * @return Le joueur et ses différents paramètres
 */
Joueur creerJoueur(const char * pseudo) {
	Joueur j;

	strcpy(j.nom, pseudo);
	j.score = 0;
	j.enVie = false;
	j.coup = rien;
	j.absent = false;

	return j;
}

/**
 * @brief Demande le coup que veut jouer le client
 * @param n Datagramme envoyé au serveur par la suite
 */
void * jouer(void * n) {
	Datagramme *data = (Datagramme*) n;

	char coup[256];
	bool fin = false;
	// Tant que le joueur n'a pas bien répondu
	do {
		// Coup du joueur ?
		printf("Indiquez votre coup ([p]ierre, [f]euille ou [c]iseaux) : ");
		scanf("%255s", coup);

		// Pierre
		if (strcmp(coup, "P") == 0 || strcmp(coup, "p") == 0
				|| strcmp(coup, "pierre") == 0) {
			data->joueurs[rangClient].coup = pierre;
			fin = true;
		}

		// Feuille
		if (strcmp(coup, "F") == 0 || strcmp(coup, "f") == 0
				|| strcmp(coup, "feuille") == 0) {
			data->joueurs[rangClient].coup = feuille;
			fin = true;
		}

		// Ciseaux
		if (strcmp(coup, "C") == 0 || strcmp(coup, "c") == 0
				|| strcmp(coup, "ciseaux") == 0) {
			data->joueurs[rangClient].coup = ciseaux;
			fin = true;
		}
	} while (!fin);
}

/**
 * @brief Main du client. Initialise la connection avec le serveur, puis fait jouer le client avec le serveur
 * @param argc Nombre d'arguments en entrée du programme. Doit être égale à 3.
 * @param arv Arguments en entrée du programme. Doit contenir l'adresse du serveur et le pseudonyme du client.
 */
int main(int argc, char **argv) {
	// Si le nombre d'argument est mauvais, on arrête le programme
	if (argc != 3) {
		perror("usage : client <adresse-serveur> <pseudonyme>");
		exit(1);
	}

	// On récupère l'ip du serveur et le pseudo du joueur passé en entrée
	host = argv[1];
	if (strlen(argv[2]) > TAILLE_MAX_NOM) {
		perror("erreur : pseudonyme trop long.");
		exit(1);
	}
	const char * pseudo = argv[2];

	// Connection avec l'adresse IP
	if ((ptr_host = gethostbyname(host)) == NULL) {
		perror(
				"erreur : impossible de trouver le serveur a partir de son adresse.");
		exit(1);
	}

	/* copie caractere par caractere des infos de ptr_host vers adresse_locale */
	bcopy((char*) ptr_host->h_addr, (char*) &adresse_locale.sin_addr,
			ptr_host->h_length);
	adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
	adresse_locale.sin_port = htons(5000);

	/* creation de la socket */
	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror(
				"erreur : impossible de creer la socket de connexion avec le serveur.");
		exit(1);
	}

	// Affichage destiné au client pour l'accueillir
	printf("\n****************** SHIFUMI ******************\n");
	printf("Bienvenu(e) %s !\n", pseudo);
	printf("Connection au serveur en cours...");

	// Informe le serveur de la connection. Si la partie est pleine, on quitte le programme.
	if (sendNouveauJoueur(creerJoueur(pseudo))) {
		printf(
				"\nLa limite de %d joueurs est atteinte. Veuillez réessayer plus tard...\n",
				NB_MAX_JOUEURS);
		close(socket_descriptor);
		return 0;
	}
	printf(" Ok.\n");

	// Indique que le joueur ne joue actuellement pas
	joue = false;

	//	Deroulement du jeu
	for (;;) {

		// Reception des données envoyées par le serveur
		Datagramme data = readDatagramme();
		rangClient = data.joueur.rang;

		// Si on est un nouveau joueur et qu'une partie est en cours, on attends la fin de la partie
		if (data.etat == attendsFinTour) {
			printf("Partie en cours...\n");
		}

		// Si il y a moins de 2 joueurs, on attend un second joueur pour jouer ensemble
		if (data.etat == enAttente || data.nbJoueurs == 1) {
			printf("En attente d'un deuxieme joueur... \n");
			data = readDatagramme();
		}

		// Si c'est la fin du tour, on est informé de l'état de la partie
		if (data.etat == finTour) {
			// Etat de la partie
			afficheJoueurs(data.joueurs, data.nbJoueurs);
			// Si on joue
			if (joue) {

				// Affichage de votre coup
				printf("Vous avez joué : %s\n",
						coupToString(data.joueurs[rangClient].coup));

				// Affichage du coup du défenseur
				int rangDefenseur = data.joueurs[rangClient].rang + 1;
				if (rangDefenseur >= data.nbJoueurs) {
					rangDefenseur = 0;
				}
				printf("%s a défendu votre coup avec : %s\n",
						data.joueurs[rangDefenseur].nom,
						coupToString(data.joueurs[rangDefenseur].coup));

				// Résultat de votre coup sur le défenseur
				if (data.joueurs[rangDefenseur].enVie) {
					printf("Dommage, il a survécu.\n");
				} else {
					printf("Bien joué, vous l'avez vaincu !\n");
				}

				// Affichage du coup de l'attaquant
				int rangAttaquant = data.joueurs[rangClient].rang - 1;
				if (rangAttaquant < 0) {
					rangAttaquant = data.nbJoueurs - 1;
				}
				printf("%s vous a attaqué avec : %s\n",
						data.joueurs[rangAttaquant].nom,
						coupToString(data.joueurs[rangAttaquant].coup));

				// Résultat de votre coup sur l'attaquant
				if (data.joueurs[rangClient].enVie) {
					printf("Ouf, vous avez survécu.\n");
				} else {
					joue = false;
					printf("Arf, vous avez perdu.\n");
				}
			}

			// On compte le nombre de joueurs en vie
			int cptEnVie = 0;
			int i, rangDuGagnant;
			i = 0;
			while (cptEnVie < 2 && i < data.nbJoueurs) {
				if (data.joueurs[i].enVie) {
					rangDuGagnant = i;
					++cptEnVie;

				}
				++i;
			}
			printf("Il y a %d joueur(s) encore dans la partie.\n", cptEnVie);

			// Fin de la partie ?
			if (cptEnVie < 2) {
				// Si oui, on indique le gagnant et on indique que c'est la fin de la partie
				data.etat = finPartie;
				data.joueur = data.joueurs[rangDuGagnant];
				if (!cptEnVie) {
					data.joueur.enVie = false;
				}
			}
			// Sinon, on lance un nouveau tour
			else {
				data.etat = debutTour;
			}
		}

		// Si c'est la fin de la partie, on affiche le gagnant et on recommence la partie
		if (data.etat == finPartie) {
			printf("\n--- Fin de la partie ---\n");

			// Si personne ne gagne
			if (!data.joueur.enVie) {
				printf("Zut, personne n'a survécu !\n");
			}

			// Si le client gagne
			else if (data.joueur.rang == data.joueurs[rangClient].rang) {
				printf("BRAVO !! Vous avez gagné la partie.\n");
				++data.joueurs[rangClient].score;
			}

			// Si le client a perdu
			else {
				printf("%s a gagné la partie !\n", data.joueur.nom);
				printf("%s a maintenant un score de %d.\n", data.joueur.nom,
						data.joueur.score + 1);
			}

			// Affiche le score actuel et indique qu'il faut faire une nouvelle partie
			printf("Votre score actuel est de %d.\n",
					data.joueurs[rangClient].score);
			data.etat = nouvellePartie;
		}

		// Si c'est une nouvelle partie, on remets tous les jouers en vie et on lance un nouveau tour
		if (data.etat == nouvellePartie) {
			printf("\n------------- Nouvelle partie -------------\n");
			data.joueurs[rangClient].enVie = true;
			joue = true;
			data.etat = debutTour;
		}

		// Si c'est un nouveau de tour, on demande le coup du joueur et on l'envoie au serveur
		if (data.etat == debutTour) {

			// On réinitialise le coup
			data.joueurs[rangClient].coup = rien;

			// Si le joueur peut encore jouer
			if (data.joueurs[rangClient].enVie) {

				// Le joueur joue avec un timeout de 10sc, le temps la réponse du joueur
				pthread_t t;
				pthread_create(&t, NULL, &jouer, &data);
				sleep(15);
				pthread_cancel(t);

				// Notifie les joueurs absents
				if (data.joueurs[rangClient].coup == rien) {
					printf("Vous êtes absent, bye bye.\n");
					close(socket_descriptor);
					return 0;
				}

			}
			// Sinon, le joueur est mort et on lui dit d'attendre
			else {
				printf(
						"Vous etes mort, veuillez attendre la fin de la partie.\n");
			}

			// Envoie du coup joué
			data.joueur = data.joueurs[rangClient];
			sendDatagramme(data);

		}
	}
	//close(socket_descriptor);
	return 0;
}
