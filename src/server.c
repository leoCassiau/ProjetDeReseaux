﻿/* server.c
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
#include <time.h>
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

/**
 * @brief Ajoute le joueur dans le tableau joueurs[NB_MAX_JOUEURS]
 * @param joueur un Joueur
 */
/* ----------------- fonctions du tableau joueurs ----------------- */
bool addJoueur(Joueur joueur) {
    // Retourne faux si le nombre de joueurs max est atteint
    if (nbJoueurs >= NB_MAX_JOUEURS) {
        return false;
    }

    // Ajout du nouveau joueur dans la liste
    joueur.rang = nbJoueurs;
    joueurs[nbJoueurs] = joueur;
    ++nbJoueurs;

    return true;
}

/**
 * @brief Retire le joueur du tableau de joueurs
 * @param j un Joueur
 * @return un booléen à vrai si l'opération est réussie, faux sinon
 */
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

/**
 * @brief Reçoit un datagramme depuis le client
 * @param socket un int identifiant une socket client
 * @return Le Datagramme reçu
 */
/*  ----------------- fonctions pour communiquer avec le client  ----------------- */
Datagramme readDatagramme(int  socket) {

	Datagramme data;

	int longueur=recv(socket, &data, sizeof(Datagramme),MSG_WAITALL);
	if (longueur == 0) {
		printf("erreur : La socket de reception a ete fermee\n");
		data.etat=erreur;
		return data;

	}else if(longueur<0){
		printf("erreur: Impossible de lire le datagramme\n");
		data.etat=erreur;
		return data;
	}

	return data;

}


/**
 * @brief Envoie un Datagramme à un client
 * @param socket_descriptor l'identifiant de la socket client
 * @param data le Datagramme à envoyer
 * @return 0 ou -1 en cas d'erreur, 1 sinon
 */
int writeDatagramme(int socket_descriptor, Datagramme data) {
	int longueur=send(socket_descriptor, &data, sizeof(Datagramme),0);
	if ((longueur <0)) {
		printf("erreur : impossible d'ecrire le message destine au client\n");
		return 0;
	}else if ((longueur == 0)) {
		printf("erreur : La socket d'envoi a ete fermee\n");
		return -1;
	}
	else return 1;
}

/**
 * @brief Accepte la connection d'un client, reçoit le Joueur envoyé, ajout du client dans le tableau si possible,
 * envoi d'un datagramme informant le client de l'ajout et de l'état de la partie
 * @param void * n, nécessaire pour les threads, non utilisé
 */
void * nouveauClient(void * n) {

    for(;;) {
        int nouv_socket_descriptor; /* [nouveau] descripteur de socket */

        /* adresse_client_courant sera renseignÃ© par accept via les infos du connect */
        if ((nouv_socket_descriptor = accept(socket_descriptor,
                                             (sockaddr*) (&adresse_client_courant), &longueur_adresse_courante))
                < 0) {
            perror("erreur : impossible d'accepter la connexion avec le client.\n");
            exit(1);
        }

        // Ajout du client en tant que joueur de la partie
        Datagramme data = readDatagramme(nouv_socket_descriptor);

        // Ajout du joueur et vérifie si la partie est pleine
        Datagramme result;
        result.partiePleine = !addJoueur(data.joueur);
        joueurs[nbJoueurs-1].socket=nouv_socket_descriptor;

        printf("Ajout du joueur : %s.\n", joueurs[nbJoueurs-1].nom);
        printf("Connection avec la socket: %d \n",joueurs[nbJoueurs-1].socket);
        printf("Il y a actuellement %d joueurs \n",nbJoueurs);
        result.joueur=joueurs[nbJoueurs-1];

        int i;
        for (i = 0; i < nbJoueurs; i++) {
            result.joueurs[i] = joueurs[i];
        }
        result.nbJoueurs = nbJoueurs;

        if(result.partiePleine){
            printf("La partie est pleine \n");
        }
        // Vérifie l'état de la partie
        else if (nbJoueurs < 2) { // Attends un deuxieme joueur
            result.etat = enAttente;
            printf("Envoi de l'etat enAttente. \n");
        } else if (nbJoueurs == 2) { // Debut du jeu
            result.etat = nouvellePartie;
            printf("Envoi de l'etat nouvellePartie. \n");
            Datagramme res;
            res.etat = nouvellePartie;
            res.nbJoueurs = nbJoueurs;
            res.joueur=joueurs[0];
            res.joueurs[0]=joueurs[0];
            printf("Annonce au joueur %s , socket %d du debut de la partie\n",joueurs[0].nom, joueurs[0].socket);
            writeDatagramme(joueurs[0].socket, res);
        } else { // Affichage du tour en cours
            result.etat = attendsFinTour;
            printf("Envoi de l'etat attendsFinTour. \n");
        }

        // Envoi du datagramme
       
        writeDatagramme(joueurs[nbJoueurs-1].socket, result);//1er envoi verifie que la partie est non pleine
        writeDatagramme(joueurs[nbJoueurs-1].socket, result);
    }
}

/**
 * @brief Reception d'un datagramme, si il est correct: mise à jour du joueur en question,
 * sinon message de debug
 * @param void * n: en réalité un int identifiant de socket client
 * @return Le Datagramme reçu
 */
void * reception(void * n) {
    // Reception du datagramme
    int *  nouv_socket_descriptor = (int*) n;
    Datagramme data = readDatagramme(*nouv_socket_descriptor);

    // Mise à jour du joueur
    if(data.etat!=erreur && nbJoueurs>1){
    printf("%s a joue le coup : %s.\n", data.joueur.nom,
           coupToString(data.joueur.coup));
    joueurs[data.joueur.rang] = data.joueur;
	}else
	printf("Reception erronee, coup non pris en compte\n");

}

/**
 * @brief Teste la validité d'une socket
 * @param sock un int identifiant une socket client
 * @return true si la socket est ouverte, false sinon
 */
bool isSocketOpen(int sock){
	int error = 0;
	socklen_t len = sizeof (error);
	int retval = getsockopt (sock, SOL_SOCKET, SO_ERROR, &error, &len);
	
	if (retval != 0) {
		return false;
	}else

	if (error != 0) {
		return false;
	}else
		return true;
	
}

/**
 * @brief Fonction main du server, lance les thread d'ajout de nouveaux joueurs
 * et de réception de datagramme, attends les réponses des clients, joue un tour et envoie les résultats aux clients
 * @param int argc (par défaut)
 * @param char **argv (par défaut)
 * @return int (par défaut)
 */
/* ----------------- main ----------------- */
int main(int argc, char **argv) {
    gethostname(machine, 256); /* recuperation du nom de la machine */

    /* recuperation de la structure d'adresse en utilisant le nom */
    if ((ptr_hote = gethostbyname(machine)) == NULL) {
        perror(
                    "erreur : impossible de trouver le serveur a partir de son nom.\n");
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

    /* attente des connexions et traitement des donnees recues */
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(
                    "erreur : impossible de creer la socket de connexion avec le client.");
        exit(1);
    }

    /* association du socket socket_descriptor Ã  la structure d'adresse adresse_locale */
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
    pthread_create(&t_nouveauClient, NULL, &nouveauClient, NULL);
	
    for (;;) {
	// On attends 2 joueurs
	while(nbJoueurs<2){}

        // reception des coups joués par les joueurs
        pthread_t t;
        pthread_t threads[NB_MAX_JOUEURS];
        int nbThreads = 0;
        int i;
		
		
        for(i=0 ; i < nbJoueurs ; i++) {
			
		
			if(isSocketOpen(joueurs[i].socket)){
		    joueurs[i].coup = rien;
		    pthread_create(&t, NULL, &reception, &joueurs[i].socket);
		    threads[nbThreads] = t;
		    ++nbThreads;
			}else
				if(removeJoueur(joueurs[i])){
					if (nbJoueurs<2){
						Datagramme dataOsef=readDatagramme(joueurs[0].socket);
					}
					printf("Joueur supprime\n");
				}
        }

        // Synchronisation
        //sleep(10);
        for (i = 0; i < nbThreads; i++) {
            pthread_join(threads[i], NULL);
	    printf("Synchronisation\n");
            //pthread_cancel(threads[i]);
        }

				i = 0;
        while(i < nbJoueurs) {
             if(joueurs[i].coup == rien && joueurs[i].enVie) {
                removeJoueur(joueurs[i]);
            }
               ++i;
        }
        Joueur joueursQuiJoue[NB_MAX_JOUEURS];
        int nbJoueursQuiJoue = 0;
        for (i = 0; i < nbJoueurs; i++) {
            if(joueurs[i].enVie) {
                joueursQuiJoue[nbJoueursQuiJoue] = joueurs[i];
                ++nbJoueursQuiJoue;
            }
        }

        // On fait jouer les joueurs entre eux
        for (i = 0; i < nbJoueursQuiJoue; i++) {
            int j = i + 1; // j le joueur suivant dans la ronde

            // Si i est le dernier joueur, alors il attaque le premier joueur
            if (j == nbJoueursQuiJoue) {
                j = 0;
            }

            attaque(&joueursQuiJoue[i], &joueursQuiJoue[j]);
        }

	// MAJ des joueurs 
	for(i=0;i<nbJoueursQuiJoue;i++){
            int rang = joueursQuiJoue[i].rang;
            joueurs[rang].enVie = joueursQuiJoue[i].enVie;
        }
        
        // On informe les joueurs des résultats
        Datagramme data;
        data.etat = finTour;

	// Ajout de joueurs dans DATA
	for (i = 0; i < nbJoueurs; i++) {
		data.joueurs[i] = joueurs[i];
	}
	data.nbJoueurs = nbJoueurs;
	
        for (i = 0; i < nbJoueurs; i++) {
		//printf("DEBUG joueur nom :%s\n",joueurs[i].nom);
			data.joueur=joueurs[i];
            if (writeDatagramme(joueurs[i].socket, data)<=0){
				if(removeJoueur(joueurs[i])){
					if (nbJoueurs<2){
						Datagramme dataOsef=readDatagramme(joueurs[0].socket);
					}
				printf("Joueur supprime\n");
				}
			}
        }
    }

    return 0;
}
