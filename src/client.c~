/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "shifumi.c"
#include <stdbool.h>

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

// Variables globales pour la connexion avec le serveur
int 	socket_descriptor; 	/* descripteur de socket */
sockaddr_in adresse_locale; 	/* adresse de socket local */
hostent *	ptr_host; 			/* info sur une machine hote */
servent *	ptr_service; 		/* info sur service */
char * host;	/* adresse de l'hôte*/

// Variables globales pour le déroulement du jeu
Joueur joueurClient;	// Etat du client actuel

void sendDatagramme(Datagramme data) {
		printf("Envoie d'un datagramme...\n");
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
			perror("erreur : impossible de se connecter au serveur.");
			exit(1);
    }

    /* envoi du message vers le serveur */
    if ((write(socket_descriptor, &data, sizeof(Datagramme)) < 0)) {
			perror("erreur : impossible d'ecrire le message destine au serveur.");
			exit(1);
    }

		printf("Datagramme envoyé.");
		close(socket_descriptor);
}

Datagramme readDatagramme() {
	Datagramme data;
	read(socket_descriptor, &data, sizeof(Datagramme));
	return data;
}

bool sendNouveauJoueur(Joueur joueur) {
	Datagramme data;
	data.operation = nouveauJoueur;
	data.joueur = joueur;
	sendDatagramme(data);
	data = readDatagramme();
	return data.partiePleine;
}

Joueur creerJoueur(const char * pseudo) {
	//TODO Manque un test si le pseudo est trop long
	Joueur j;
	strcpy(j.nom,pseudo);
	j.score = 0;
	j.enVie = false;
	j.coup = rien;

	return j;
}
 
void jouer() {
	char coup[256];
	bool fin = false;
	do {
		// Coup du joueur ?
		printf("Indiquez votre coup (Pierre, Feuille ou Ciseaux) : ");
		scanf("%255s", coup);
		printf("\n");
		
		// Pierre
		if(strcmp(coup, "P")==0 || strcmp(coup, "p")==0  || strcmp(coup, "Pierre")==0) {
			joueurClient.coup=pierre;
			//sendJoueurInfo(joueurClient);
			fin = true;
		}
	
		// Feuille
		if(strcmp(coup, "F")==0|| strcmp(coup, "f")==0 || strcmp(coup, "Feuille")==0) {
			joueurClient.coup=feuille;
			//sendJoueurInfo(joueurClient);
			fin = true;
		}
	
		// Ciseaux
		if(strcmp(coup, "C")==0 || strcmp(coup, "c")==0  || strcmp(coup, "Ciseaux")==0) {
			joueurClient.coup=ciseaux;
			//sendJoueurInfo(joueurClient);
			fin = true;
		}
	} while(!fin);
}

etatPartie readEtatPartie() {
		return debut;
}

int main(int argc, char **argv) {
		// Initialisation
    if (argc != 3) {
			perror("usage : client <adresse-serveur> <pseudonyme>");
			exit(1);
    }
    host = argv[1];
    const char * pseudo = argv[2];
	
		if ((ptr_host = gethostbyname(host)) == NULL) {
			perror("erreur : impossible de trouver le serveur a partir de son adresse.");
			exit(1);
  	}
    
    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
    adresse_locale.sin_port = htons(5000);

    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("erreur : impossible de creer la socket de connexion avec le serveur.");
			exit(1);
    }

		// Creation du joueur
		printf("\n****************** SHIFUMI ******************\n");
		printf("\n Bienvenu(e) %s\n", pseudo);
		joueurClient = creerJoueur(pseudo);
		printf("\nConnection au serveur en cours... \n");
		if(!sendNouveauJoueur(joueurClient)) {
			printf("La limite de %d joueurs est atteinte. Veuillez ressayer plus tard... \n", NB_MAX_JOUEURS);
			return 0;
		}
		printf("Ok \n");

		//	Deroulement du jeu
		for(;;){
			etatPartie etat = readEtatPartie();

			if(etat == enCours) {
				if(joueurClient.enVie == false) {
					//Joueur joueurs[NB_MAX_JOUEURS] = readEtatPartie();
					//afficheJoueurs(joueurs);
				} else {
					jouer();
				}
			}

			if(etat == enAttente) {
				printf("En attente d'un deuxieme joueur... \n");
				//TODO 
				readDatagramme();
			}

			if(etat == debut) {
				printf("------------- Nouvelle partie -------------\n");
				joueurClient.enVie = true;
			}
	}
	return 0;
}
