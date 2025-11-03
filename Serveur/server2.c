#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "server2.h"
#include "../jeu.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static int end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void clear_clients(Client *clients, int actual);
static void remove_client(Client *clients, int to_remove, int *actual);

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;
    int max = sock;
    Client clients[MAX_CLIENTS];
    Partie parties[MAX_GAMES];
    Challenge challenges[MAX_CLIENTS];
    int nb_challenges = 0;
    int nb_parties = 0;

    fd_set rdfs;

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        for (int i = 0; i < actual; i++)
            FD_SET(clients[i].sock, &rdfs);

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        /* Sortie console serveur */
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            printf("Stopping server...\n");
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            /* Nouveau client */
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            /* Lecture du pseudo */
            if (read_client(csock, buffer) <= 0)
                continue;

            Client c = {0};
            c.sock = csock;
            c.id_playing_game = -1;
            c.id_observing_game = -1;
            strncpy(c.name, buffer, sizeof(c.name) - 1);

            clients[actual++] = c;
            printf("Client connected: %s\n", c.name);

            char welcome[BUF_SIZE];
            snprintf(welcome, sizeof(welcome), "Welcome %s! Type /list or /challenge <name> to start.%s",
                     c.name, CRLF);
            write_client(c.sock, welcome);

            max = csock > max ? csock : max;
        }
        else
        {
            /* Un client parle */
            for (int i = 0; i < actual; i++)
            {
                Client *cli = &clients[i];
                if (FD_ISSET(cli->sock, &rdfs))
                {
                    int n = read_client(cli->sock, buffer);
                    if (n <= 0)
                    {
                        printf("%s disconnected.\n", cli->name);
                        closesocket(cli->sock);
                        remove_client(clients, i, &actual);
                        break;
                    }

                    /* Traitement commande */
                    if (strncmp(buffer, "/list", 5) == 0)
                    {
                        send_user_list(clients, actual, i);
                    }
                    else if (strncmp(buffer, "/challenge", 10) == 0)
                    {
                        char target[50];
                        sscanf(buffer, "/challenge %49s", target);
                        handle_challenge(clients, challenges, &nb_challenges, actual, i, target);
                    }
                    else if (strncmp(buffer, "/accept", 7) == 0)
                    {
                        char challenger_name[50];
                        sscanf(buffer, "/accept %49s", challenger_name);
                        handle_accept(clients, parties, &nb_parties, challenges, &nb_challenges, actual, i, challenger_name);
                    }
                    else if (strncmp(buffer, "/refuse", 7) == 0)
                    {
                        char challenger_name[50];
                        sscanf(buffer, "/refuse %49s", challenger_name);
                        handle_refuse(clients, actual, challenges, &nb_challenges, i, challenger_name);
                    }
                    else if (strncmp(buffer, "/move", 5) == 0)
                    {
                        char move_str[10];
                        sscanf(buffer, "/move %9s", move_str);
                        handle_move(clients, parties, nb_parties, i, move_str);
                    }
                    else
                    {
                        write_client(cli->sock, "Unknown command.\r\n");
                    }
                    break;
                }
            }
        }
    }

    clear_clients(clients, actual);
    end_connection(sock);
}


static int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

static int end_connection(int sock)
{
    if (closesocket(sock) == 0) {
        return 1;
    } else {
        perror("closesocket");
        return 0; 
    }
}


static int read_client(SOCKET sock, char *buffer)
{
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0)
    {
        perror("recv()");
        n = 0;
    }
    buffer[n] = 0;
    return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

static void clear_clients(Client *clients, int actual)
{
    for (int i = 0; i < actual; i++)
        closesocket(clients[i].sock);
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
    memmove(clients + to_remove, clients + to_remove + 1,
            (*actual - to_remove - 1) * sizeof(Client));
    (*actual)--;
}

void register_client(Client *clients, int idx, const char *pseudo)
{
    strncpy(clients[idx].name, pseudo, sizeof(clients[idx].name) - 1);
    clients[idx].id_playing_game = -1;
    clients[idx].id_observing_game = -1;
}

void send_user_list(Client *clients, int actual, int to)
{
    char buffer[BUF_SIZE];
    buffer[0] = 0;
    strcat(buffer, "Online users: ");
    for (int i = 0; i < actual; i++)
    {
        strcat(buffer, clients[i].name);
        if (i < actual - 1)
            strcat(buffer, ", ");
    }
    write_client(clients[to].sock, buffer);
}

int find_client_by_name(Client *clients, int actual, const char *name)
{
    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, name) == 0)
            return i;
    }
    return -1;
}

void handle_challenge(Client *clients, Challenge *challenges, int *nb_challenges, int actual, int challenger_idx, const char *target)
{
    int target_idx = find_client_by_name(clients, actual, target);
    if (target_idx < 0)
    {
        write_client(clients[challenger_idx].sock, "Target not found.");
        return;
    }

    for (int i = 0; i < *nb_challenges; i++)
    {
        if (challenges[i].active &&
            challenges[i].challenger_idx == challenger_idx &&
            challenges[i].target_idx == target_idx)
        {
            write_client(clients[challenger_idx].sock, "Challenge already sent.");
            return;
        }
    }

    int idx = *nb_challenges;
    challenges[idx].challenger_idx = challenger_idx;
    challenges[idx].target_idx = target_idx;
    challenges[idx].active = 1;
    (*nb_challenges)++;

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s has challenged you! Type /accept %s or /refuse %s",
             clients[challenger_idx].name, clients[challenger_idx].name, clients[challenger_idx].name);
    write_client(clients[target_idx].sock, msg);

    write_client(clients[challenger_idx].sock, "Challenge sent.");
}

void handle_accept(Client *clients, Partie *parties, int *nb_parties,
                   Challenge *challenges, int *nb_challenges, int actual,
                   int accepter_idx, const char *challenger)
{
    int challenger_idx = -1;
    int challenge_idx = -1;

    for (int i = 0; i < *nb_challenges; i++)
    {
        if (challenges[i].active &&
            challenges[i].target_idx == accepter_idx &&
            strcmp(clients[challenges[i].challenger_idx].name, challenger) == 0)
        {
            challenger_idx = challenges[i].challenger_idx;
            challenge_idx = i;
            break;
        }
    }

    if (challenger_idx == -1)
    {
        write_client(clients[accepter_idx].sock, "Challenger not found.");
        return;
    }

    int game_id = create_game(parties, nb_parties, challenger_idx, accepter_idx);

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "Game started between %s and %s. %s starts first.",
             clients[challenger_idx].name, clients[accepter_idx].name,
             (rand() % 2 == 0) ? clients[challenger_idx].name : clients[accepter_idx].name);

    write_client(clients[challenger_idx].sock, msg);
    write_client(clients[accepter_idx].sock, msg);

    char plateau[BUF_SIZE];
    afficherPlateau(&parties[game_id].jeu, plateau, sizeof(plateau));
    strncat(plateau, "\nJouez /move n pour jouer la case n.\r\n", sizeof(plateau) - strlen(plateau) - 1);

    write_client(clients[challenger_idx].sock, plateau);
    write_client(clients[accepter_idx].sock, plateau);

    challenges[challenge_idx].active = 0;
}

void handle_refuse(Client *clients, int actual, Challenge *challenges, int *nb_challenges,
                   int refuser_idx, const char *challenger)
{
    int challenger_idx = -1;
    int challenge_idx = -1;

    for (int i = 0; i < *nb_challenges; i++)
    {
        if (challenges[i].active &&
            challenges[i].target_idx == refuser_idx &&
            strcmp(clients[challenges[i].challenger_idx].name, challenger) == 0)
        {
            challenger_idx = challenges[i].challenger_idx;
            challenge_idx = i;
            break;
        }
    }

    if (challenger_idx == -1)
    {
        write_client(clients[refuser_idx].sock, "Challenger not found.");
        return;
    }

    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "%s refused your challenge.", clients[refuser_idx].name);
    write_client(clients[challenger_idx].sock, msg);

    write_client(clients[refuser_idx].sock, "You refused the challenge. That's so sad :(");

    challenges[challenge_idx].active = 0;
}

int create_game(Partie *parties, int *nb_parties, int playerA, int playerB)
{
    int id = *nb_parties;
    parties[id].id = id;
    initPlateau(&parties[id].jeu);
    parties[id].playerA = playerA;
    parties[id].playerB = playerB;
    parties[id].nbObservers = 0;
    parties[id].active = 1;
    (*nb_parties)++;
    return id;
}

void handle_move(Client *clients, Partie *parties, int nb_parties, int player_idx, const char *move)
{
    int game_idx = -1;
    for (int i = 0; i < nb_parties; i++)
    {
        if (parties[i].active && (parties[i].playerA == player_idx || parties[i].playerB == player_idx))
        {
            game_idx = i;
            break;
        }
    }
    if (game_idx < 0)
    {
        write_client(clients[player_idx].sock, "You are not in a game.");
        return;
    }

    int index = atoi(move);
    if (!jouerCoup(&parties[game_idx].jeu, player_idx, index))
    {
        write_client(clients[player_idx].sock, "Invalid move.");
        return;
    }

    broadcast_game_state(&parties[game_idx], clients);
}

void broadcast_game_state(Partie *p, Client *clients)
{
    char buffer[BUF_SIZE];

    afficherPlateau(&p->jeu, buffer, sizeof(buffer));

    write_client(clients[p->playerA].sock, buffer);
    write_client(clients[p->playerB].sock, buffer);

    for (int i = 0; i < p->nbObservers; i++)
    {
        write_client(clients[p->observers[i]].sock, buffer);
    }
}

/* ======================== MAIN ======================== */

int main(int argc, char **argv)
{
    init();
    printf("=== Awale Server started on port %d ===\n", PORT);
    app();
    end();
    printf("=== Server stopped ===\n");
    return EXIT_SUCCESS;
}
