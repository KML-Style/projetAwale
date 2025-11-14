#ifndef VAR_H
#define VAR_H

#include <sys/select.h>
#include "jeu.h"

#define MAX_CLIENTS   20
#define MAX_GAMES     (MAX_CLIENTS/2)
#define MAX_ACCOUNTS  256
#define BUF_SIZE      512
#define PORT          8080
#define USERS_FILE    "data/users.txt"

typedef struct {
    char username[16];
    char password[32];
    char bio[512];
    char friends[256];
} Account;

typedef struct {
    int fd;
    char name[16];
    int loggedIn;
    int inGame;
    int opponent;
    int playerIndex;
    int ready;
    int loginStage;
    int privateMode;
} Client;

typedef struct {
    int active;
    int plateau[12];
    Joueur j0, j1;
    int toMove;
    char filename[160];
    int playerFd0, playerFd1;
    int observers[MAX_CLIENTS];
    int observerCount;
} Game;

extern Client  clients[MAX_CLIENTS];
extern Game    games[MAX_GAMES];
extern Account accounts[MAX_ACCOUNTS];
extern int     accountCount;

extern fd_set master_set;
extern int max_fd;

#endif

