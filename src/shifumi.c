#include <string.h>

using namespace std;

enum coup {
    pierre,
    feuille,
    ciseaux
};

struct joueur {
    string nom; // pseudo
    coup c;     // coup joué
    int score;  // score actuel du joueur
};

bool bat(coup c1, coup c2) {
    if( c1 == pierre && c2 == ciseaux ||
        c1 == ciseaux && c2 == feuille ||
        c1 == feuille && c2 == pierre) {
        return true;
    }
    return false;
}

bool attaque(joueur j1, joueur j2) {
    if(bat(j1.c, j2.c)) {
        return true;
    }
    return false;
}

bool defend(joueur j1, joueur j2) {
    if(bat(j2.c, j1.c)) {
        return true;
    }
    return false;
}


