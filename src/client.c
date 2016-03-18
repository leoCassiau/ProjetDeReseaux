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
bool joue;

void sendDatagramme(Datagramme data) {
    int longueur=send(socket_descriptor, &data, sizeof(Datagramme),0);
    /* envoi du message vers le serveur */
    if ((longueur < 0)) {
        printf("erreur : impossible d'ecrire le message destine au serveur.");
        exit(1);
    }else if ((longueur== 0)) {
        printf("erreur : La socket d'envoi a ete fermee.");
        exit(1);
    }
}

Datagramme readDatagramme() {
    Datagramme data;
    int longueur=recv(socket_descriptor, &data, sizeof(Datagramme),MSG_WAITALL);
    if ((longueur< 0)) {
        printf("erreur : impossible de lire le message recu.");
        exit(1);
    }else if ((longueur== 0)) {
        printf("erreur : La socket de reception a ete fermee.");
        exit(1);
    }

    return data;
}

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

Joueur creerJoueur(const char * pseudo) {
    Joueur j;
    strcpy(j.nom, pseudo);
    j.score = 0;
    j.enVie = false;
    j.coup = rien;
        j.absent = false;

    return j;
}

void * jouer(void * n) {
    Datagramme *data = (Datagramme*) n;

    char coup[256];
    bool fin = false;
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
  //  printf("DEBUG data.jouers[0] socket : %d \n", data.joueurs[0].socket);
   //printf("DEBUG data.jouers[0] rang : %d,  %s \n", rangClient, coupToString(data.joueurs[rangClient].coup));
}

int main(int argc, char **argv) {
    // Initialisation
    if (argc != 3) {
        perror("usage : client <adresse-serveur> <pseudonyme>");
        exit(1);
    }
    host = argv[1];
    if(strlen(argv[2]) > TAILLE_MAX_NOM) {
        perror("erreur : pseudonyme trop long.");
        exit(1);
    }
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
    printf("Connection au serveur en cours...");
    if (sendNouveauJoueur(creerJoueur(pseudo))) {
        printf(
                "\nLa limite de %d joueurs est atteinte. Veuillez réessayer plus tard...\n",
                NB_MAX_JOUEURS);

        return 0;
    }
    printf(" Ok.\n");

    joue = false;

	//	Deroulement du jeu
	for (;;) {
		
		Datagramme data = readDatagramme();
		
		rangClient=data.joueur.rang;
		printf("Rang recu: %d \n", rangClient);
        if(data.etat == attendsFinTour) {
            printf("Partie en cours...\n");
        }

		if (data.etat == enAttente||data.nbJoueurs==1) {
			printf("En attente d'un deuxieme joueur... \n");
            data = readDatagramme();
		}


        if (data.etat == finTour) {
            afficheJoueurs(data.joueurs, data.nbJoueurs);
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
                //printf("DEBUG nom : %s, enVie : %d, nbJoueur : %d, cptEnVie : %d\n",
                //	data.joueurs[i].nom, data.joueurs[i].enVie, data.nbJoueurs, cptEnVie);
                if (data.joueurs[i].enVie) {
                    rangDuGagnant = i;
                    ++cptEnVie;

                }
                ++i;
            }
            printf("Il y a %d joueur(s) encore dans la partie.\n", cptEnVie);

            // Fin de la partie ?
            if(cptEnVie < 2) {
                data.etat = finPartie;
                data.joueur = data.joueurs[rangDuGagnant];
                if(!cptEnVie) {
                    data.joueur.enVie = false;
                }
            } else {
                data.etat = debutTour;
            }
        }

        if (data.etat == finPartie) {
            printf("\n--- Fin de la partie ---\n");

            // Si personne ne gagne
            if(!data.joueur.enVie) {
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
                        data.joueur.score+1);
            }


            printf("Votre score actuel est de %d.\n", data.joueurs[rangClient].score);
            data.etat = nouvellePartie;
        }

        if (data.etat == nouvellePartie) {
            printf("\n------------- Nouvelle partie -------------\n");
            data.joueurs[rangClient].enVie = true;
            joue = true;
            data.etat = debutTour;
        }

        if (data.etat == debutTour) {
            data.joueurs[rangClient].coup = rien;
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
            }else
                printf("Vous etes mort, veuillez attendre la fin de la partie.\n");

            // Envoie du coup joué
            data.joueur = data.joueurs[rangClient];
            sendDatagramme(data);

        }
    }
    //close(socket_descriptor);
    return 0;
}
