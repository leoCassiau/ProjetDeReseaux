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
int nbJoueursAlive=0;

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
Datagramme readDatagramme(int  socket) {
    Datagramme data;
    int longueur=read(socket, &data, sizeof(Datagramme));
    if (longueur == 0) {
        perror("erreur : La socket de reception a ete fermee\n");
        exit(1);
    }else if(longueur<0){
        perror("erreur: Impossible de lire le datagramme\n");
        exit(1);
    }else

        return data;
}


void writeDatagramme(int socket_descriptor, Datagramme data) {
    int longueur=write(socket_descriptor, &data, sizeof(Datagramme));
    if ((longueur <0)) {
        perror("erreur : impossible d'ecrire le message destine au client\n");
        exit(1);
    }else if ((longueur == 0)) {
        perror("erreur : La socket d'envoi a ete fermee\n");
        exit(1);
    }

}

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
        data.joueur.socket=nouv_socket_descriptor;

        // Ajout du joueur et vérifie si la partie est pleine
        Datagramme result;
        result.partiePleine = !addJoueur(data.joueur);

        printf("Ajout du joueur : %s.\n", data.joueur.nom);
        printf("Connection avec la socket: %d \n",data.joueur.socket);
        printf("Il y a actuellement %d joueurs \n",nbJoueurs);
        result.joueur=joueurs[nbJoueurs-1];
        if(result.partiePleine){
            printf("DEBUG: La partie est pleine \n");
        }
        // Vérifie l'état de la partie
        if (nbJoueurs < 2) { // Attends un deuxieme joueur
            result.etat = enAttente;
            printf("DEBUG: Envoi de l'etat enAttente. \n");
        } else if (nbJoueurs == 2) { // Debut du jeu
            result.etat = nouvellePartie;
            printf("DEBUG: Envoi de l'etat nouvellePartie. \n");
            writeDatagramme(joueurs[0].socket, result);
        } else { // Affichage du tour en cours
            result.etat = finTour;
            printf("DEBUG: Envoi de l'etat finTour. \n");
        }

        // Envoi du datagramme
        writeDatagramme(data.joueur.socket, result);//1er envoi verifie que la partie est non pleine
    }
}

void * reception(void * n) {
    // Reception du datagramme
    int *  nouv_socket_descriptor = (int*) n;

    printf("DEBUG: socket %d \n", *nouv_socket_descriptor);
    Datagramme data = readDatagramme(*nouv_socket_descriptor);


    // Mise à jour du joueur
    printf("%s a joue le coup : %s.\n", data.joueur.nom,
           coupToString(data.joueur.coup));
    joueurs[data.joueur.rang] = data.joueur;

}

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
    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 0;

    if( setsockopt(socket_descriptor, SOL_SOCKET,  SO_RCVTIMEO,(void *)(&t), sizeof(t)) < 0)
    {
        printf("socket failed\n");
        close(socket_descriptor);
        exit(2);
    }
    /* initialisation de la file d'ecoute */
    listen(socket_descriptor, 5);

    longueur_adresse_courante = sizeof(adresse_client_courant);

    // Thread gérant les nouveaux joueurs.
    pthread_t t_nouveauClient;
    pthread_create(&t_nouveauClient, NULL, &nouveauClient, NULL);

    for (;;) {

        // reception des coups joués par les joueurs
        pthread_t t;
        pthread_t threads[NB_MAX_JOUEURS];
        int nbThreads = 0;
        int i;

        for(i=0 ; i < nbJoueurs ; i++) {
            pthread_create(&t, NULL, &reception, &joueurs[i].socket);
            threads[nbThreads] = t;
            ++nbThreads;
        }

        printf("nb de thread: %d \n",nbThreads);
        // Synchronisation
        for (i = 0; i < nbThreads; i++) {
            printf("synchronisation\n");
            pthread_join(threads[i],NULL);

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
            int counter;
            for(counter=0;counter<=nbJoueurs;counter++){
                data.joueurs[counter] = joueurs[counter];
            }
            data.nbJoueurs = nbJoueurs;
            for(counter=0;counter<=nbJoueurs;counter++){
                if(joueurs[i].enVie){
                    data.joueur = joueurs[counter];
                    //writeDatagramme(joueurs[i].socket, data);
                }
            }
        }
    }

    return 0;
}
