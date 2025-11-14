#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include "var.h"
#include "gestionClient.h"
#include "gestionPartie.h"
#include "jeu.h"

int main() {
    mkdir("data", 0777);
    mkdir("games", 0777);

    accountCount = loadAccounts();
    printf("%d compte(s) utilisateur chargé(s).\n", accountCount);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); exit(1);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); exit(1);
    }

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
    for (int g = 0; g < MAX_GAMES; g++) games[g].active = 0;

    printf("Awalé server fonctionne sur le port %d...\n", PORT);

    while (1) {
        fd_set read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select"); break;
        }

        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    handleNewConnection(server_fd);
                } else {
                    handleClientMessage(i);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}

