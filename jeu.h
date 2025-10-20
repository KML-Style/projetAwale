#ifndef JEU_H
#define JEU_H


#include <stdio.h>
#include <stdbool.h>

#define CASES 12

typedef struct {
    int plateau[CASES];
    int joueur;
    int score[2]
} Jeu;

void init(Jeu* j);
bool jouerCoup(Jeu* j, int joueur, int indexCase);
void afficherPlateau(Jeu* j);
bool verifierFin(Jeu* j);

#endif //JEU_H