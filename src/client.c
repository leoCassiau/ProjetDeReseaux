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
Joueur joueurClient;	// Etat du client actuel

void sendDatagramme(Datagramme data) {
	printf("Envoi d'un datagramme...\n");
	int longueur=send(socket_descriptor, &data, sizeof(Datagramme),0);
	/* envoi du message vers le serveur */
	if ((longueur < 0)) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
	}else if ((longueur== 0)) {
		perror("erreur : La socket d'envoi a ete fermee.");
		exit(1);
	}else

	printf("Datagramme envoyé.");
}

Datagramme readDatagramme() {
	Datagramme data;
	
	int longueur=recv(socket_descriptor, &data, sizeof(Datagramme),NULL);
	if ((longueur< 0)) {
		perror("erreur : impossible de lire le message recu.");
		exit(1);
	}else if ((longueur== 0)) {
		perror("erreur : La socket de reception a ete fermee.");
		exit(1);
	}
	
	return data;
}

bool sendNouveauJoueur(Joueur joueur) {
	Datagramme data;
	data.joueur = joueur;
	/* tentative de connexion au serveur dont les infos sont dans adresse_locale */
	if ((connect(socket_descriptor, (sockaddr*) (&adresse_locale),
			sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.");
		exit(1);
	}
	printf("socketdescriptor: %d",socket_descriptor);
	sendDatagramme(data);
	
	data = readDatagramme();
    joueurClient = data.joueur;
	if (data.partiePleine){
		printf("DEBUG: la partie est pleine \n");
	}
	return data.partiePleine;
}

Joueur creerJoueur(const char * pseudo) {
	Joueur j;
	strcpy(j.nom, pseudo);
	j.score = 0;
	j.enVie = false;
	j.coup = rien;
	j.absent = false;
	//j.socket = socket_descriptor;

	return j;
}

void * jouer(void * n) {
	char coup[256];
	bool fin = false;
	do {
		// Coup du joueur ?
		printf("Indiquez votre coup (Pierre, Feuille ou Ciseaux) : ");
		scanf("%255s", coup);
		printf("\n");

		// Pierre
		if (strcmp(coup, "P") == 0 || strcmp(coup, "p") == 0
				|| strcmp(coup, "Pierre") == 0) {
			joueurClient.coup = pierre;
			fin = true;
		}

		// Feuille
		if (strcmp(coup, "F") == 0 || strcmp(coup, "f") == 0
				|| strcmp(coup, "Feuille") == 0) {
			joueurClient.coup = feuille;
			fin = true;
		}

		// Ciseaux
		if (strcmp(coup, "C") == 0 || strcmp(coup, "c") == 0
				|| strcmp(coup, "Ciseaux") == 0) {
			joueurClient.coup = ciseaux;
			fin = true;
		}
	} while (!fin);
}

int main(int argc, char **argv) {
	// Initialisation
	if (argc != 3) {
		perror("usage : client <adresse-serveur> <pseudonyme>");
		exit(1);
	}
	host = argv[1];
	//TODO Test si size(argv[2]) < TAILLE_MAX_NOM
	const char * pseudo = argv[2];

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

	// Creation du joueur
	printf("\n****************** SHIFUMI ******************\n");
	printf("Bienvenu(e) %s !\n", pseudo);
	joueurClient = creerJoueur(pseudo);
	printf("Connection au serveur en cours...");
	if (sendNouveauJoueur(joueurClient)) {
		printf(
				"\nLa limite de %d joueurs est atteinte. Veuillez réessayer plus tard...\n",
				NB_MAX_JOUEURS);
				
		return 0;
	}
	
	
	//	Deroulement du jeu
	for (;;) {
		
		Datagramme data = readDatagramme();
		joueurClient=data.joueur;
		printf(" Tu as le num de socket: %d\n",joueurClient.socket);
		printf(" Ton nom est: %s\n",joueurClient.nom);
		printf(" Ton score est: %d\n",joueurClient.score);
		if (data.etat == enAttente) {
			printf("En attente d'un deuxieme joueur... \n");
			data = readDatagramme();
		}

		if (data.etat == finTour) {
			//afficheJoueurs(data.joueurs, data.nbJoueurs);
			if (joueurClient.enVie) {

				// Affichage de votre coup
				printf("%Vous avez joué : %s.\n",
						coupToString(joueurClient.coup));

				// Affichage du coup du défenseur
				int rangDefenseur = joueurClient.rang + 1;
				if (rangDefenseur >= data.nbJoueurs) {
					rangDefenseur = 0;
				}
				//printf("%s a défendu votre coup avec : %s.\n",
				//		data.joueurs[rangDefenseur].nom,
				//		coupToString(data.joueurs[rangDefenseur].coup));

				// Résultat de votre coup sur le défenseur
				if (data.joueurs[rangDefenseur].enVie) {
					printf("Dommage, il a survécu.\n");
				} else {
					printf("Bien joué, vous l'avez vaincu !\n");
				}

				// Affichage du coup de l'attaquant
				int rangAttaquant = joueurClient.rang - 1;
				if (rangAttaquant < 0) {
					rangAttaquant = data.nbJoueurs - 1;
				}
				//printf("%s vous a attaqué avec : %s.\n",
				//		data.joueurs[rangAttaquant].nom,
				//		coupToString(data.joueurs[rangAttaquant].coup));

				// Résultat de votre coup sur l'attaquant
				if (data.joueur.enVie) {
					printf("Ouf, vous avez survécu.\n");
				} else {
					printf("Arf, vous avez perdu.\n");
					joueurClient.enVie = false;
				}
			}

			// On compte le nombre de joueurs en vie
			int cptEnVie, i, rangDuGagnant;
			cptEnVie, i = 0;
			while (cptEnVie < 2 && i < data.nbJoueurs) {
				if (data.joueurs[i].enVie) {
					rangDuGagnant = i;
					++cptEnVie;
				}
				++i;
			}

			// Fin de la partie ?
			if(cptEnVie < 2) {
				data.etat = finPartie;
				data.joueur = data.joueurs[rangDuGagnant];
			} else {
				data.etat = debutTour;
			}
		}

		if (data.etat == finPartie) {
			printf("\n--- Fin de la partie ---\n");

			// Si le client gagne
			if (data.joueur.rang == joueurClient.rang) {
			//	printf("BRAVO !! Vous avez gagné la partie.\n");
				++joueurClient.score;
			}

			// Si le client a perdu
			else {
			//	printf("%s a gagné la partie !\n", data.joueur.nom);
			//	printf("%s a maintenant un score de %d.\n", data.joueur.nom,
			//			data.joueur.score);
			}

			//printf("Votre score actuel est de %s.\n", joueurClient.score);
			data.etat = nouvellePartie;
		}

		if (data.etat == nouvellePartie) {
			printf("\n------------- Nouvelle partie -------------\n");
			joueurClient.enVie = true;
			data.etat = debutTour;
		}

		if (data.etat == debutTour) {
			if (joueurClient.enVie) {
				joueurClient.coup = rien;

				// Le joueur joue avec un timeout de 10sc, le temps la réponse du joueur
				pthread_t t;
				pthread_create(&t, NULL, &jouer, NULL);
				sleep(10);
				pthread_cancel(t);

				// Notifie les joueurs absents
				if (joueurClient.coup == rien) {
					joueurClient.absent = true;
				}

				// Envoie du coup joué
				Datagramme dataSend;
				dataSend.joueur = joueurClient;
				sendDatagramme(dataSend);
			}
		}

		if (joueurClient.absent) {
			printf("Vous êtes absent, bye bye.\n");
			close(socket_descriptor);
			return 0;
		}
	}
	close(socket_descriptor);
	return 0;
}
