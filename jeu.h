#ifndef JEU_H
#define JEU_H

#include <stdio.h>

// Joueur
typedef struct {
    int numero;         // 0 ou 1, en fonction de sa position dans la partie
    char pseudo[16];    
    int score;          
} Joueur;

// Initialiser le plateau de 
void initialiserJeu(int plateau[]);

// Réinitialiser les scores 
void reinitialiserScores(Joueur *joueur0, Joueur *joueur1);

// Détecter une situation de famine et retourne -1 si aucune famine (le numéro du joueur affamé sinon)
int detecterFamine(int plateau[]);

// Vérifier si la partie est terminée et retourne 1 si fin de partie (0 sinon)
int terminerPartie(int plateau[], int *score0, int *score1);

// Appelé par jouerCoup : Gèrer la capture de graines après un coup et mettre à jour les scores et le plateau
void captureGraines(int plateau[], int numero, Joueur *joueur0, Joueur *joueur1, int derniereCase);

// Jouer un coup pour le joueur donné et mettre à jour le plateau après le coup
// 0: OK, 1-5 : coups illégaux, 6 : coup interdit car il ne nourrit pas l’adversaire en famine
int jouerCoup(int plateau[], int numero, Joueur *joueur0, Joueur *joueur1, int indice);

// Afficher le plateau et les scores
void afficherPlateau(int plateau[], Joueur joueur0, Joueur joueur1);

#endif
