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


int 	socket_descriptor, 	/* descripteur de socket */
		longueur; 		/* longueur d'un buffer utilisÃ© */
sockaddr_in adresse_locale; 	/* adresse de socket local */
hostent *	ptr_host; 		/* info sur une machine hote */
servent *	ptr_service; 		/* info sur service */
char 	buffer[256];
//char *	prog; 			/* nom du programme */
char *	host; 			/* nom de la machine distante */
char * pseudo; 

/*---------------------Notre code----------------------*/
#define NB_MAX_JOUEURS 256
int nbJoueurs = 0;
Joueur joueurs[NB_MAX_JOUEURS];
Joueur joueurClient;
bool peutJouer=false; 
bool lirePseudo=true;

void sendJoueurInfo(Joueur player){
	if ((ptr_host = gethostbyname(host)) == NULL) {
	perror("erreur : impossible de trouver le serveur a partir de son adresse.");
	exit(1);
    }
    
    /* copie caractere par caractere des infos de ptr_host vers adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */
    
    /* 2 facons de definir le service que l'on va utiliser a distance */
    /* (commenter l'une ou l'autre des solutions) */
    
    /*-----------------------------------------------------------*/
    /* SOLUTION 1 : utiliser un service existant, par ex. "irc" */
    /*
    if ((ptr_service = getservbyname("irc","tcp")) == NULL) {
	perror("erreur : impossible de recuperer le numero de port du service desire.");
	exit(1);
    }
    adresse_locale.sin_port = htons(ptr_service->s_port);
    */
    /*-----------------------------------------------------------*/
    
    /*-----------------------------------------------------------*/
    /* SOLUTION 2 : utiliser un nouveau numero de port */
    adresse_locale.sin_port = htons(5000);
    /*-----------------------------------------------------------*/
    
    
    /* creation de la socket */
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("erreur : impossible de creer la socket de connexion avec le serveur.");
	exit(1);
    }
    
    /* tentative de connexion au serveur dont les infos sont dans adresse_locale */
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
	perror("erreur : impossible de se connecter au serveur.");
	exit(1);
    }
  
      
    /* envoi du message vers le serveur */
    if ((write(socket_descriptor, &player, sizeof(Joueur)) < 0)) {
	perror("erreur : impossible d'ecrire le message destine au serveur.");
	exit(1);
    }
   
    
    /* mise en attente du prgramme pour simuler un delai de transmission */
    sleep(2);
                
    /* lecture de la reponse en provenance du serveur */
    Joueur joueursListe[NB_MAX_JOUEURS];
    while((longueur = read(socket_descriptor, &joueursListe, sizeof(Joueur[NB_MAX_JOUEURS]))) > 0) {
		;
	for(int i=0;i<=256;i++){	
		//joueurs[i].c=joueursListe[i].c;
		//joueurs[i].isAlive=joueursListe[i].isAlive;
		//joueurs[i].score=joueursListe[i].score;
		//joueurs[i].rank=joueursListe[i].rank;
		//strcpy(joueurs[i].nom,joueursListe[i].nom);
		joueurs[i]=joueursListe[i];
		if(strcmp(joueurs[i].nom,joueurClient.nom)==0){
			joueurClient.c=joueurs[i].c;
			joueurClient.rank=i;
			joueurClient.score=joueurs[i].score;
			joueurClient.isAlive=joueurs[i].isAlive;
		}
		}
    }
    
    
   
    
    
   
    
    
  
    close(socket_descriptor);
    
  
    
    //exit(0);
}


void nouveauJoueur() {
	
	pseudo = malloc(sizeof(char) * 256);
	printf("\n****************** SHIFUMI ******************\n");
	printf("Bienvenu(e), veuillez indiquer votre pseudonyme : ");
	scanf("%s", pseudo);
	printf("\n Bienvenu(e), %s \n",pseudo);
	
	Joueur j;
	
	strcpy(j.nom,pseudo);
	j.score=-1;
	printf("\nConnection au serveur en cours... \n");
	joueurClient=j;
	sendJoueurInfo(j);
	
	// TODO Informer le serveur du nouveau joueur
	// La réponse : affiche la partie en cours
	// Ou on attends un joueur
	//printf("En attente d'un second joueur... \n");
	// Attends la réponse du serveur
}

void nouveauTour() {
	char coup[256];
	joueurClient.aJoue=false;
	do {
		printf("Indiquez votre coup (Pierre, Feuille ou Ciseaux) : ");
		scanf("%255s", coup);
		
		printf("\n");
		
		if(strcmp(coup, "P")==0 || strcmp(coup, "p")==0  || strcmp(coup, "Pierre")==0){
		joueurClient.c=pierre;
		peutJouer=false;
		joueurClient.aJoue=true;
		sendJoueurInfo(joueurClient);
			}
	
	if(strcmp(coup, "F")==0|| strcmp(coup, "f")==0 || strcmp(coup, "Feuille")==0){
		joueurClient.c=feuille;
		peutJouer=false;
		joueurClient.aJoue=true;
		sendJoueurInfo(joueurClient);
			}
	
	if(strcmp(coup, "C")==0 || strcmp(coup, "c")==0  || strcmp(coup, "Ciseaux")==0){
		joueurClient.c=ciseaux;
		peutJouer=false;
		joueurClient.aJoue=true;
		sendJoueurInfo(joueurClient);
}
	} while((strcmp(coup, "P") != 0 || strcmp(coup, "p") != 0 || strcmp(coup, "Pierre") != 0 ||
	 		strcmp(coup, "F") != 0 || strcmp(coup, "f") != 0 || strcmp(coup, "Feuille") != 0 ||
	 		strcmp(coup, "C") != 0 || strcmp(coup, "c") != 0 || strcmp(coup, "Ciseaux") != 0 )&&peutJouer==true);
	
		
}
void nouvellePartie() {
	printf("------------- Nouvelle partie -------------\n");
	joueurClient.isAlive=true;
	peutJouer=true;
	printf("score : %i \n",joueurClient.score);
	while(peutJouer){
		if(joueurClient.isAlive==true){
				printf("Tu es encore vivant.  \n");
			}else if(joueurClient.isAlive==false)
				printf("Tu es  MORT.  \n");
			printf("Ton rang dans la ronde est:    %i\n",joueurClient.rank);
			printf("Ton score est :  %i \n",joueurClient.score);
			
		nouveauTour();
		sleep(10);
		peutJouer=true;
		
	}
}



void finTour() {
	//TODO Reception du serveur
}


int main(int argc, char **argv) {
  
    
     
    /*if (argc != 3) {
	perror("usage : client <adresse-serveur> <message-a-transmettre>");
	exit(1);
    }
   
    prog = argv[0];
    
    mesg = argv[2];
    *
    printf("nom de l'executable : %s \n", prog);
    printf("adresse du serveur  : %s \n", host);
    printf("message envoye      : %s \n", mesg);
    */
    //host="localhost";
    host = argv[1];
    for (;;){
	
		if(lirePseudo){
			nouveauJoueur();
			lirePseudo=false;
			
			printf("NOM :  %20s \n",joueurClient.nom);
			printf("Ton rang dans la ronde est:    %i\n",joueurClient.rank);
			printf("Ton score est :  %i \n",joueurClient.score);
			
			nouvellePartie();
			}
		else if(joueurClient.isAlive==false){
			
			char reponse[2];
			do{
			printf(" Voulez-vous rejouer? (y/n) \n");
			scanf("%2s", reponse);
			printf("\n");}
			while(strcmp(reponse,"y")!=0);
			
			//CRADE
			
			nouvellePartie();
		}	
		
	}
    
}
