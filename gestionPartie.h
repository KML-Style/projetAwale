#ifndef GESTIONPARTIE_H
#define GESTIONPARTIE_H

#include "var.h"

// Gestion des parties
void sendBoard(Game *g);
void startGame(int a, int b);
void endGame(Game *g);
void processMove(int ci, int pit);
int canObserve(Game *g, const char *observer);
void cancelClientGame(int ci);
int findGameByPlayer(char *player);

#endif

