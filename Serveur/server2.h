#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_GAMES   40

#define BUF_SIZE    1024

#include "../Client/client2.h"
#include "../jeu.h"

typedef struct {
    SOCKET sock;
    char name[50];
    int id_playing_game;      // id de la partie (-1 si libre)
    int id_observing_game;    // id de la partie observée (-1 si aucune)
} Client;

typedef struct {
    int id;
    Jeu jeu;
    int playerA;      // index du joueur A dans le tableau clients[]
    int playerB;      // index du joueur B dans le tableau clients[]
    int observers[10];
    int nbObservers;
    bool active;
} Partie;

typedef struct {
    int challenger_idx;  
    int target_idx;      
    char active;         // 1 si actif, 0 sinon
} Challenge;

void register_client(Client *clients, int idx, const char *pseudo);
void send_user_list(Client *clients, int actual, int to);
int find_client_by_name(Client *clients, int actual, const char *name);

void handle_challenge(Client *clients, Challenge *challenges, int *nb_challenges, int actual, int challenger_idx, const char *target);
void handle_accept(Client *clients, Partie *parties, int *nb_parties, Challenge *challenges, int *nb_challenges, int actual,int accepter_idx, const char *challenger);
void handle_refuse(Client *clients, int actual, Challenge *challenges, int *nb_challenges, int refuser_idx, const char *challenger);

void handle_move(Client *clients, Partie *parties, int nb_parties, int player_idx, const char *move);
void broadcast_game_state(Partie *p, Client *clients);
int create_game(Partie *parties, int *nb_parties, int playerA, int playerB);

#endif /* guard */