#include "jeu.h"

void initPlateau(Jeu* j){
    for (int i=0; i<CASES; i++){
        j->plateau[i] = 4;
    }

    for (int i=0; i<2; i++){
        j->score[i] = 0;
    }

    j->joueur = 0;
    
}

bool jouerCoup(Jeu* j, int joueur, int indexCase){

    //Test : le joueur qui joue doit être le joueur qui a le trait
    if (joueur != j->joueur){
        return false;
    }

    // Test : le joueur doit jouer une case appartenant à "son plateau"
    int debutP1 = (joueur == 0) ? 0 : 6;
    int finP1   = (joueur == 0) ? 5 : 11;
    if (indexCase < debutP1 || indexCase > finP1) {
        return false;
    }

    // Test : le joueur ne peut pas jouer une indexCase contenant aucune graine
    if (j->plateau[indexCase] == 0){
        return false;
    }

    // Distribution des graines 
    int graines = j->plateau[indexCase];
    j->plateau[indexCase] = 0;

    for (int i=1; i<= graines; i++){
        j->plateau[(indexCase + i) % CASES]++;
    }
    

    // Prises éventuelles
    int derniereCase = (indexCase + graines) % CASES;
    int captures = 0;

    int debut = (joueur == 0) ? 6 : 0;
    int fin   = (joueur == 0) ? 11 : 5;

    for (int i = derniereCase; (joueur == 0 && i >= debut) || (joueur == 1 && i <= fin); i += (joueur == 0 ? -1 : 1)) {
        if (j->plateau[i] == 2 || j->plateau[i] == 3) {
            captures += j->plateau[i];
            j->plateau[i] = 0;
        } else break;
    }

    // Gestion du score
    j->score[joueur] += captures;

    // Changement de trait
    j->joueur = (j->joueur + 1) % 2;

    return true;
}

void afficherPlateau(Jeu* j, char* buffer, size_t bufsize) {
    char tmp[256];
    buffer[0] = '\0';  // initialisation

    strncat(buffer, "\n============================================\n\n", bufsize - strlen(buffer) - 1);
    strncat(buffer, "Joueur 2 →\n", bufsize - strlen(buffer) - 1);
    strncat(buffer, "   Cases : ", bufsize - strlen(buffer) - 1);

    for (int i = 11; i >= 6; i--) {
        snprintf(tmp, sizeof(tmp), " %2d  ", i);
        strncat(buffer, tmp, bufsize - strlen(buffer) - 1);
    }

    strncat(buffer, "\n          ", bufsize - strlen(buffer) - 1);
    for (int i = 11; i >= 6; i--) {
        snprintf(tmp, sizeof(tmp), " [%2d] ", j->plateau[i]);
        strncat(buffer, tmp, bufsize - strlen(buffer) - 1);
    }
    strncat(buffer, "\n   ", bufsize - strlen(buffer) - 1);
    for (int i = 0; i <= 5; i++) {
        snprintf(tmp, sizeof(tmp), " [%2d] ", j->plateau[i]);
        strncat(buffer, tmp, bufsize - strlen(buffer) - 1);
    }
    strncat(buffer, "\n   Cases : ", bufsize - strlen(buffer) - 1);
    for (int i = 0; i <= 5; i++) {
        snprintf(tmp, sizeof(tmp), " %2d  ", i);
        strncat(buffer, tmp, bufsize - strlen(buffer) - 1);
    }
    strncat(buffer, "\nJoueur 1 →\n", bufsize - strlen(buffer) - 1);

    snprintf(tmp, sizeof(tmp), "\nScores : J1 = %d | J2 = %d\nTour du joueur : %d\n=============================================\n\n",
             j->score[0], j->score[1], j->joueur + 1);
    strncat(buffer, tmp, bufsize - strlen(buffer) - 1);
}


bool verifierFin(Jeu* j){
    // Fin : un joueur a 25 graines ou plus
    if (j->score[0] >= 25 || j->score[1] >= 25) return true;

    // Fin : l'adversaire est en famine
    int adversaire = (j->joueur + 1) % 2;
    int debut = (adversaire == 0) ? 0 : 6;
    int fin   = (adversaire == 0) ? 5 : 11;

    for (int i = debut; i <= fin; i++) {
        if (j->plateau[i] > 0) return false; 
    }
    return true;
}
