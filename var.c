#include "var.h"

Client  clients[MAX_CLIENTS];
Game    games[MAX_GAMES];
Account accounts[MAX_ACCOUNTS];
int     accountCount = 0;
int     clientsCount = 0;

fd_set master_set;
int max_fd;

