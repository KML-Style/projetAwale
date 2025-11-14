#include "gestionPartie.h"
#include "compte.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "jeu.h"

void sendBoard(Game *g) {
    char msg[256];
    snprintf(msg, sizeof(msg),
             "PLATEAU %d %d %d %d %d %d %d %d %d %d %d %d | Scores: %d-%d | Prochain: %d\n",
             g->plateau[0], g->plateau[1], g->plateau[2], g->plateau[3],
             g->plateau[4], g->plateau[5], g->plateau[6], g->plateau[7],
             g->plateau[8], g->plateau[9], g->plateau[10], g->plateau[11],
             g->j0.score, g->j1.score, g->toMove);
    if (g->playerFd0 > 0) send(g->playerFd0, msg, strlen(msg), 0);
    if (g->playerFd1 > 0) send(g->playerFd1, msg, strlen(msg), 0);
    for (int z = 0; z < g->observerCount; z++)
        if (g->observers[z] != -1) send(g->observers[z], msg, strlen(msg), 0);
}

void startGame(int a, int b) {
    int gidx = -1;
    for (int i = 0; i < MAX_GAMES; i++) if (!games[i].active) { gidx = i; break; }
    if (gidx == -1) return;
    Game *g = &games[gidx];
    g->active = 1;
    g->observerCount = 0;
    initialiserJeu(g->plateau);
    reinitialiserScores(&g->j0, &g->j1);
    strncpy(g->j0.pseudo, clients[a].name, 15); g->j0.pseudo[15]='\0';
    strncpy(g->j1.pseudo, clients[b].name, 15); g->j1.pseudo[15]='\0';
    g->toMove = rand() % 2;
    g->playerFd0 = clients[a].fd;
    g->playerFd1 = clients[b].fd;

    clients[a].inGame = clients[b].inGame = 1;
    clients[a].opponent = b; clients[b].opponent = a;
    clients[a].playerIndex = 0; clients[b].playerIndex = 1;
    clients[a].ready = clients[b].ready = 0;

    time_t t = time(NULL);
    char ts[32]; strftime(ts, sizeof(ts), "%Y-%m-%d_%H-%M-%S", localtime(&t));
    snprintf(g->filename, sizeof(g->filename),
             "games/game_%s_vs_%s_%s.txt", clients[a].name, clients[b].name, ts);
    FILE *f = fopen(g->filename, "w");
    if (f) { fprintf(f, "DEBUT DE LA PARTIE %s vs %s\n", clients[a].name, clients[b].name); fclose(f); }

    char msg[128];
    snprintf(msg, sizeof(msg), "DEBUT DE LA PARTIE %s vs %s\n", clients[a].name, clients[b].name);
    send(clients[a].fd, msg, strlen(msg), 0);
    send(clients[b].fd, msg, strlen(msg), 0);
}

void endGame(Game *g) {
    char endmsg[128];
    snprintf(endmsg, sizeof(endmsg), "FIN DE LA PARTIE : %d - %d\n", g->j0.score, g->j1.score);
    if (g->playerFd0 > 0) send(g->playerFd0, endmsg, strlen(endmsg), 0);
    if (g->playerFd1 > 0) send(g->playerFd1, endmsg, strlen(endmsg), 0);
    for (int z = 0; z < g->observerCount; z++)
        if (g->observers[z] != -1) send(g->observers[z], endmsg, strlen(endmsg), 0);

    FILE *f = fopen(g->filename, "a");
    if (f) {
        fprintf(f, "FIN DE LA PARTIE, %s: %d - %s: %d\n",
                g->j0.pseudo, g->j0.score, g->j1.pseudo, g->j1.score);
        fclose(f);
    }
    g->active = 0;
}

void processMove(int ci, int pit) {
        if (!clients[ci].inGame) { send(clients[ci].fd, "ERREUR : Impossible de jouer des coups en dehors d'une partie.\n", strlen("ERREUR : Impossible de jouer des coups en dehors d'une partie.\n"), 0); return; }

    Game *g = NULL;
    for (int k = 0; k < MAX_GAMES; k++) {
        if (games[k].active &&
            (strcmp(games[k].j0.pseudo, clients[ci].name) == 0 ||
             strcmp(games[k].j1.pseudo, clients[ci].name) == 0)) {
            g = &games[k]; break;
        }
    }
    if (!g) { send(clients[ci].fd, "ERREUR : Pas de partie trouvée.\n", strlen("ERREUR : Pas de partie trouvée.\n"), 0); return; }

    if (clients[ci].playerIndex != g->toMove) {
        send(clients[ci].fd, "ERREUR : C'est au tour de l'adversaire.\n", strlen("ERREUR : C'est au tour de l'adversaire.\n"), 0); return;
    }

    int rc = jouerCoup(g->plateau, clients[ci].playerIndex, &g->j0, &g->j1, pit);
    if (rc != 0) { send(clients[ci].fd, "ATTENTION : Ce coup est illégal !\n", strlen("ATTENTION : Ce coup est illégal !\n"), 0); return; }

    FILE *f = fopen(g->filename, "a");
    if (f) { fprintf(f, "%s;%d\n", clients[ci].name, pit); fclose(f); }

    g->toMove ^= 1;
    sendBoard(g);

    if (terminerPartie(g->plateau, &g->j0.score, &g->j1.score)) {
        endGame(g);
    }
}

int canObserve(Game *g, const char *observer) {
    int c0 = clientIndexByName(g->j0.pseudo);
    int c1 = clientIndexByName(g->j1.pseudo);
    int priv0 = (c0 >= 0) ? clients[c0].privateMode : 0;
    int priv1 = (c1 >= 0) ? clients[c1].privateMode : 0;
    if (!priv0 && !priv1) return 1;

    int a0 = findAccount(g->j0.pseudo);
    int a1 = findAccount(g->j1.pseudo);

    if (priv0 && (a0 < 0 || !isFriend(&accounts[a0], observer))) return 0;
    if (priv1 && (a1 < 0 || !isFriend(&accounts[a1], observer))) return 0;
    return 1;
}

void cancelClientGame(int ci) {
    int opp = clients[ci].opponent;
    if (ci < 0 || ci >= MAX_CLIENTS) return;
    if (clients[ci].inGame && opp >= 0 && clients[opp].fd != -1) {
        char msg[64]; snprintf(msg, sizeof(msg), "INFO %s a abandonné la partie.\n", clients[ci].name);
        send(clients[opp].fd, msg, strlen(msg), 0);
        clients[opp].inGame = 0; clients[opp].opponent = -1; clients[opp].ready = 0;
    }
    for (int g = 0; g < MAX_GAMES; g++)
        if (games[g].active && (strcmp(games[g].j0.pseudo, clients[ci].name)==0 ||
                                strcmp(games[g].j1.pseudo, clients[ci].name)==0))
            games[g].active = 0;
    clients[ci].inGame = 0; clients[ci].opponent = -1; clients[ci].ready = 0;
}

int findGameByPlayer(char *player)
{
    for(int i=0;i<MAX_GAMES;i++)
    {
        if(games[i].active && (strcmp(games[i].j0.pseudo,player)==0 || strcmp(games[i].j1.pseudo,player)==0))
        {
            return i;
        }

    }
    return -1;
}