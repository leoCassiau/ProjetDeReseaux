/* server.c
 * Serveur du jeu Shifumi a lancer avant le client
 *
 * Auteurs : Léo Cassiau, Jean-Christophe Guérin
 * Mars 2016
 *
 * Faculté des sciences et techniques - Master ALMA
 * Projet de Réseaux encadré par M. Passard et Mme Hamma
 */

#include "shifumi.c"
#include <pthread.h>	// Parallelisation
#include <linux/types.h> 	/* pour les sockets */
#include <sys/socket.h>
#include <netdb.h> 		/* pour hostent, servent */

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

// Variables globales utilisées pour la connexion avec le serveur
int socket_descriptor, /* descripteur de socket */
longueur_adresse_courante; /* longueur d'adresse courante d'un client */
sockaddr_in adresse_locale, /* structure d'adresse locale*/
adresse_client_courant; /* adresse client courant */
hostent* ptr_hote; /* les infos recuperees sur la machine hote */
servent* ptr_service; /* les infos recuperees sur le service de la machine */
char machine[256]; /* nom de la machine locale */

// Variables globales pour le déroulement du jeu
int nbJoueurs = 0;
Joueur joueurs[NB_MAX_JOUEURS];
int nbJoueursAlive;

/* ----------------- fonctions du tableau joueurs ----------------- */
bool addJoueur(Joueur joueur) {
	// Retourne faux si le nombre de joueurs max est atteint
	if (nbJoueurs >= NB_MAX_JOUEURS) {
		return false;
	}

	// Ajout du nouveau joueur dans la liste
	joueurs[nbJoueurs] = joueur;
	++nbJoueurs;

	return true;
}

bool removeJoueur(Joueur j) {
	// Le joueur est identifié par son rang, càd son indice dans le tableau joueurs
	// Si son rang est supérieur à la taille de joueurs, alors c'est une erreur
	if (j.rang >= nbJoueurs) {
		return false;
	}

	// On décrémente l'indice de tous les joueurs suivant le joueur supprimé
	int i;
	for (i = j.rang; i < nbJoueurs; i++) {
		joueurs[i] = joueurs[i + 1];
		--joueurs[i].rang;
	}
	--nbJoueurs;

	return true;
}

/*  ----------------- fonctions pour communiquer avec le client  ----------------- */
Datagramme readDatagramme(int * socket_descriptor) {
	Datagramme data;
	int longueur;
	read(socket_descriptor, &data, sizeof(Datagramme));
	return data;
}

void writeDatagramme(int * socket_descriptor, Datagramme data) {
	write(socket_descriptor, &data, sizeof(Datagramme));
}

void * nouveauClient(void * n) {
	int nouv_socket_descriptor; /* [nouveau] descripteur de socket */

	/* adresse_client_courant sera renseignÃ© par accept via les infos du connect */
	if ((nouv_socket_descriptor = accept(socket_descriptor,
			(sockaddr*) (&adresse_client_courant), &longueur_adresse_courante))
			< 0) {
		perror("erreur : impossible d'accepter la connexion avec le client.");
		exit(1);
	}

	// Ajout du client en tant que joueur de la partie
	Datagramme data = readDatagramme(nouv_socket_descriptor);
	printf("Ajout du joueur : %s.\n", data.joueur.nom);

	// Ajout du joueur et vérifie si la partie est pleine
	Datagramme result;
	result.partiePleine = !addJoueur(data.joueur);

	// Vérifie l'état de la partie
	if (nbJoueurs < 2) { // Attends un deuxieme joueur
		result.etat = enAttente;
	} else if (nbJoueurs == 2) { // Debut du jeu
		result.etat = nouvellePartie;
		writeDatagramme(joueurs[0].socket, result);
	} else { // Affichage du tour en cours
		result.etat = finTour;
	}

	// Envoie du datagramme
	writeDatagramme(socket_descriptor, result);
}

void * reception(void * n) {
	// Reception du datagramme
	int * nouv_socket_descriptor = (int*) n;
	Datagramme data = readDatagramme(nouv_socket_descriptor);

	// Mise à jour du joueur
	printf("%s a joué le coup : %s.\n", data.joueur.nom,
			coupToString(data.joueur.coup));
	joueurs[data.joueur.rang] = data.joueur;
}

/* ----------------- main ----------------- */
int main(int argc, char **argv) {
	gethostname(machine, 256); /* recuperation du nom de la machine */

	/* recuperation de la structure d'adresse en utilisant le nom */
	if ((ptr_hote = gethostbyname(machine)) == NULL) {
		perror(
				"erreur : impossible de trouver le serveur a partir de son nom.");
		exit(1);
	}

	/* initialisation de la structure adresse_locale avec les infos recuperees */

	/* copie de ptr_hote vers adresse_locale */
	bcopy((char*) ptr_hote->h_addr, (char*) &adresse_locale.sin_addr,
			ptr_hote->h_length);
	adresse_locale.sin_family = ptr_hote->h_addrtype; /* ou AF_INET */
	adresse_locale.sin_addr.s_addr = INADDR_ANY; /* ou AF_INET */

	// port du serveur
	adresse_locale.sin_port = htons(5000);
	printf("numero de port pour la connexion au serveur : %d \n",
			ntohs(adresse_locale.sin_port) /*ntohs(ptr_service->s_port)*/);

	/* creation de la socket */
	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror(
				"erreur : impossible de creer la socket de connexion avec le client.");
		exit(1);
	}

	/* association du socket socket_descriptor Ã  la structure d'adresse adresse_locale */
	if ((bind(socket_descriptor, (sockaddr*) (&adresse_locale),
			sizeof(adresse_locale))) < 0) {
		perror(
				"erreur : impossible de lier la socket a l'adresse de connexion.");
		exit(1);
	}

	/* initialisation de la file d'ecoute */
	listen(socket_descriptor, 5);

	longueur_adresse_courante = sizeof(adresse_client_courant);

	// Thread gérant les nouveaux joueurs.
	pthread_t t_nouveauClient;
	pthread_create(&t_nouveauClient, NULL, nouveauClient, NULL);

	/* attente des connexions et traitement des donnees recues */
	for (;;) {

		// reception des coups joués par les joueurs
		pthread_t t;
		pthreads_t threads[NB_MAX_JOUEURS];
		int nbThreads = 0;
		int i;
		for(i=0 ; i < nbJoueurs ; i++) {
			pthread_create(&t, NULL, reception, &joueurs[i].socket);
			threads[nbThreads] = t;
			++nbThreads;
		}

		// Synchronisation
		for (i = 0; i < nbThreads; i++) {
			pthread_join(&threads[i]);
		}

		// On fait jouer les joueurs entre eux
		for (i = 0; i < nbJoueurs; i++) {
			int j = i + 1; // j le joueur suivant dans la ronde

			// Si i est le dernier joueur, alors il attaque le premier joueur
			if (i == (nbJoueurs - 1)) {
				j = 0;
			}

			attaque(&joueurs[i], &joueurs[j]);
		}

		// On informe les joueurs des résultats
		for (i = 0; i < nbJoueurs; i++) {
			Datagramme data;
			data.etat = finTour;
			data.joueurs = joueurs;
			data.nbJoueurs = nbJoueurs;
			sendDatagramme(joueurs[i].socket, data);
		}
	}

	return 0;
}
