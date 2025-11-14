#ifndef GESTIONCLIENT_H
#define GESTIONCLIENT_H

#include "var.h"

// Gestion des clients
void broadcast(const char *msg, int except_fd);
int clientIndexByName(const char *name);
void removeClient(int fd);
void handleNewConnection(int server_fd);
void handleClientMessage(int fd);
void initClients(void);
int createServerSocket(int port);
void runServerLoop(int server_fd);

#endif

