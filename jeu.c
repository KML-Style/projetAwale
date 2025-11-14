#include "jeu.h"

// Fonctions d'initialisation du plateau et des scores
void initialiserJeu(int plateau[]) {
    for (int i = 0; i < 12; i++) {
        plateau[i] = 4;
    }
}

void reinitialiserScores(Joueur *joueur0, Joueur *joueur1) {
    joueur0->score = 0;
    joueur1->score = 0;
}

// Détecte une famine (0 : joueur 0 affamé, 1 : joueur 1 affamé, -1 : aucune famine)
int detecterFamine(int plateau[]) {
    int grainesJ0 = 0, grainesJ1 = 0;

    for (int i = 0; i <= 5; i++) grainesJ0 += plateau[i];
    for (int i = 6; i <= 11; i++) grainesJ1 += plateau[i];

    if (grainesJ0 == 0 && grainesJ1 == 0) return -1;  // on enlève le cas du plateau vide
    if (grainesJ0 == 0) return 0;
    if (grainesJ1 == 0) return 1;

    return -1;
}

// Vérifie si la partie doit se terminer (1 si la partie est finie, 0 sinon)
int terminerPartie(int plateau[], int *score0, int *score1) {
    // Un joueur a >= 25 points
    if (*score0 > 24 || *score1 > 24)
        return 1;

    // >= 48 points au total 
    if ((*score0 + *score1) >= 48)
        return 1;

    // Famine que l'on ne peut pas nourrir
    int grainesJ0 = 0, grainesJ1 = 0;
    for (int i = 0; i <= 5; i++) grainesJ0 += plateau[i];
    for (int i = 6; i <= 11; i++) grainesJ1 += plateau[i];

    if (detecterFamine(plateau) != -1) {
        int joueurVide = (grainesJ0 == 0) ? 0 : 1;
        int joueurAdverse = (joueurVide == 0) ? 1 : 0;

        int peutNourrir = 0;
        if (joueurAdverse == 0) {
            for (int i = 0; i <= 5; i++) {
                if (plateau[i] > (5 - i)) {
                    peutNourrir = 1;
                    break;
                }
            }
        } else {
            for (int i = 6; i <= 11; i++) {
                if (plateau[i] > (i - 5)) {
                    peutNourrir = 1;
                    break;
                }
            }
        }

        if (!peutNourrir) {
            int grainesRestantes = grainesJ0 + grainesJ1;

            if (joueurVide == 0)
                *score1 += grainesRestantes;
            else
                *score0 += grainesRestantes;

            for (int i = 0; i < 12; i++) plateau[i] = 0;
            return 1;
        }
    }
    return 0;
}

// Capture des graines 
void captureGraines(int plateau[], int numero, Joueur *joueur0, Joueur *joueur1, int derniereCase) {
    int sens = (numero == 0) ? -1 : 1;
    int i = derniereCase;
    int captures = 0;
    int nbCaptures = 0;
    int indicesCaptures[12];

    // Cases capturables
    while (i >= 0 && i <= 11 &&((numero == 0 && i >= 6) || (numero == 1 && i <= 5)) && (plateau[i] == 2 || plateau[i] == 3)) {
        captures += plateau[i];
        indicesCaptures[nbCaptures++] = i;
        i += sens;
    }

    // Vérifie qu’il reste des graines dans le camp adverse après capture
    int grainesRestantes = 0;
    for (int j = (numero == 0 ? 6 : 0); j <= (numero == 0 ? 11 : 5); j++) {
        int valeur = plateau[j];
        for (int k = 0; k < nbCaptures; k++)
            if (indicesCaptures[k] == j) valeur = 0;
        grainesRestantes += valeur;
    }

    // Applique la capture si elle n’affame pas l’adversaire
    if (grainesRestantes > 0) {
        for (int k = 0; k < nbCaptures; k++)
            plateau[indicesCaptures[k]] = 0;

        if (numero == 0) joueur0->score += captures;
        else joueur1->score += captures;
    }
}

// Le joueur joue un coup 
int jouerCoup(int plateau[], int numero, Joueur *joueur0, Joueur *joueur1, int indice) {
    // 1-5 : coups illégaux
    if (numero != 0 && numero != 1) return 1;
    if (indice < 0 || indice > 11) return 2;
    if (numero == 0 && indice > 5) return 3;
    if (numero == 1 && indice <= 5) return 4;
    if (plateau[indice] == 0) return 5;

    int graines = plateau[indice];

    // 6 : on ne nourrit pas l'adversaire qui est en famine
    if (detecterFamine(plateau)!= -1) {
        if ((numero == 0 && graines <= (5 - indice)) || (numero == 1 && graines <= (11 - indice))) return 6;
    }

    // Distribution des graines 
    plateau[indice] = 0;
    int pos = indice;
    while (graines > 0) {
        pos = (pos + 1) % 12;
        if (pos == indice) continue;
        plateau[pos]++;
        graines--;
    }

    // Capture éventuelle
    captureGraines(plateau, numero, joueur0, joueur1, pos);

    // 0 : le coup a été joué
    return 0;
}

// Affiche le plateau et les scores
void afficherPlateau(int plateau[], Joueur joueur0, Joueur joueur1) {
    printf("%-15s : ", joueur1.pseudo);
    for (int i = 11; i >= 6; i--)
        printf(" %2d ", plateau[i]);
    printf("| Score : %d\n\n", joueur1.score);

    printf("%-15s : ", joueur0.pseudo);
    for (int i = 0; i <= 5; i++)
        printf(" %2d ", plateau[i]);
    printf("| Score : %d\n\n", joueur0.score);
}