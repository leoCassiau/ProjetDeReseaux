/*----------------------------------------------
Serveur a  lancer avant le client
------------------------------------------------*/
#include "shifumi.c"	// Struct Joueur
#include <pthread.h>	// Parallelisation
#include <stdbool.h>

#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <netdb.h> 		/* pour hostent, servent */
#include <string.h> 		/* pour bcopy, ... */  

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

// Variables globales pour la connexion avec le serveur
int socket_descriptor, 		/* descripteur de socket */
	nouv_socket_descriptor, 	/* [nouveau] descripteur de socket */
	longueur_adresse_courante; 	/* longueur d'adresse courante d'un client */
sockaddr_in adresse_locale, 		/* structure d'adresse locale*/
	adresse_client_courant; 	/* adresse client courant */
hostent* ptr_hote; 			/* les infos recuperees sur la machine hote */
servent* ptr_service; 			/* les infos recuperees sur le service de la machine */
char machine[256]; 	/* nom de la machine locale */

// Variables globales pour le déroulement du jeu
int nbJoueurs = 0;
Joueur joueurs[NB_MAX_JOUEURS];
int nbJoueursAlive;

bool addJoueur(Joueur joueur) {
	if(nbJoueurs >= NB_MAX_JOUEURS) {
		return false;
	}

	//Initialisation du nouveau joueur
	Joueur j;
	strcpy(j.nom, joueur.nom);
	j.score = 0;
	j.enVie = false;
	j.rank = nbJoueurs;

	//Ajout du nouveau joueur dans la liste
	joueurs[nbJoueurs]=j;
	++nbJoueurs;

	return true;
}

bool removeJoueur(Joueur j) {
	int i;
	bool trouve = false;

	for(i = 0 ; i < nbJoueurs ; i++) {
		if(!trouve) {
			if(joueurs[i].nom == j.nom) {
				joueurs[i] = joueurs[i + 1];
				trouve = true;
			}
		} else {
			joueurs[i] = joueurs[i + 1];
		}
	}

	if(trouve) {
		--nbJoueurs;
	}

	return trouve;
}

void nouvellePartie() {
	int i;
	for(i = 0 ; i < nbJoueurs ; i++) {
		joueurs[i].enVie = true;
		// TODO dire au joueur de jouer
	}
	nbJoueursAlive = nbJoueurs;
}

void finPartie() {
	if(nbJoueursAlive < 2) {
		if(nbJoueursAlive == 0) {
			//TODO On informe qu'il n'y a pas de gagnant
		} else { //nbJoueursAlive = 1
			for(int i = 0 ;  i < nbJoueurs ; i++) {
				if(joueurs[i].enVie) {
					joueurs[i].score++;
					// TODO Informer à tous le monde que joueurs[i] a gagné
					return;
				}
			}
		}

	}
}

void nouveauTour() {
	for(int i = 0 ; i < nbJoueurs ; i++) {
		if(joueurs[i].enVie) {
			// TODO dire au joueur de jouer
		} else {
			// TODO Dire au joueur de patienter
		}
	}
}

bool pretACalculer(){
	for(int i=0;i<=nbJoueurs;i++){
		if (joueurs[i].aJoue==false) {
			return false;
		}
		else{
		return true;
		}
	}
}
// A lancer que quand tous les joueurs ont signaler leur coup (fin timer)
void finTour() {
	if(attaque(joueurs[nbJoueurs - 1], joueurs[0])) {
		joueurs[0].enVie = false;
		nbJoueursAlive--;
		// TODO On informe à joueurs[0] sa mort
	}
	for(int i = 1 ; i < nbJoueurs ; i++) {
		if(attaque(joueurs[i], joueurs[i + 1])) {
			joueurs[i + 1].enVie = false;
			nbJoueursAlive--;
			// TODO On informe à joueurs[i+1] sa mort
		}
	}

	finPartie();
	nouvellePartie();
}

Datagramme readDatagramme(int * socket_descriptor) {
		Datagramme data;
		int longueur;
		read(socket_descriptor, &data, sizeof(Datagramme));
		return data;
}

void writeDatagramme(int * socket_descriptor, Datagramme data) {
		write(socket_descriptor, &data, sizeof(Datagramme));
}

void * fils(void * n) {
	int * socket_descriptor = (int*) n;
	for(;;) {
		Datagramme data = readDatagramme(socket_descriptor);

		if(data.operation == nouveauJoueur) {
			printf("Ajout du joueur : %s\n", data.joueur.nom);
			Datagramme result;
			result.partiePleine = !addJoueur(data.joueur);
			writeDatagramme(socket_descriptor, result);
		}
	}
}

/*------------------------------------------------------*/
int main(int argc, char **argv) {
     
		// Initialisation
    gethostname(machine,256);		/* recuperation du nom de la machine */
    
    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
    }
    
    /* initialisation de la structure adresse_locale avec les infos recuperees */			
    
    /* copie de ptr_hote vers adresse_locale */
    bcopy((char*)ptr_hote->h_addr, (char*)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family		= ptr_hote->h_addrtype; 	/* ou AF_INET */
    adresse_locale.sin_addr.s_addr	= INADDR_ANY; 			/* ou AF_INET */

		// port du serveur
    adresse_locale.sin_port = htons(5000);
    printf("numero de port pour la connexion au serveur : %d \n", 
		   ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/);
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
    }

    /* association du socket socket_descriptor Ã  la structure d'adresse adresse_locale */
    if ((bind(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
    }
    
    /* initialisation de la file d'ecoute */
    listen(socket_descriptor,5);

    /* attente des connexions et traitement des donnees recues */
    for(;;) {
		
			longueur_adresse_courante = sizeof(adresse_client_courant);
		
			/* adresse_client_courant sera renseignÃ© par accept via les infos du connect */
			if ((nouv_socket_descriptor = 
				accept(socket_descriptor, 
					     (sockaddr*)(&adresse_client_courant),
					     &longueur_adresse_courante))
				 < 0) {
				perror("erreur : impossible d'accepter la connexion avec le client.");
				exit(1);
			}

		  pthread_t t;
		      // Compilation : gcc -o toto.exe server.c -lpthread
		      // gcc -o server server.c -pthread -g -fpermissive -std=c99 -D_GNU_SOURCE

		  pthread_create(&t, NULL, fils, &nouv_socket_descriptor);
	
    }

	return 0;
}
