#include "gestionClient.h"
#include "gestionPartie.h"
#include "compte.h"
#include "var.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

void initClients(void) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;
}

void broadcast(const char *msg, int except_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1 && clients[i].loggedIn && clients[i].fd != except_fd)
            send(clients[i].fd, msg, strlen(msg), 0);
}

int clientIndexByName(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].fd != -1 && clients[i].loggedIn && strcmp(clients[i].name, name)==0)
            return i;
    return -1;
}

void removeClient(int fd) {
    for (int g = 0; g < MAX_GAMES; g++) if (games[g].active) {
        for (int z = 0; z < games[g].observerCount; z++) {
            if (games[g].observers[z] == fd) {
                games[g].observers[z] = games[g].observers[--games[g].observerCount];
                break;
            }
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) if (clients[i].fd == fd) {
        if (clients[i].inGame) cancelClientGame(i); 
        close(fd);
        FD_CLR(fd, &master_set);
        clients[i].fd = -1;
        clients[i].loggedIn = 0;
        clients[i].loginStage = 0;
        clients[i].inGame = 0;
        clients[i].ready = 0;
        clients[i].privateMode = 0;
        clients[i].opponent = -1;
        clients[i].name[0] = '\0';
        return;
    }
}

void handleNewConnection(int server_fd) {
    struct sockaddr_in cli; socklen_t alen = sizeof(cli);
    int newfd = accept(server_fd, (struct sockaddr*)&cli, &alen);
    if (newfd < 0) return;

    if (clients[clientsCount].fd == -1) {
        clients[clientsCount].fd = newfd;
        clients[clientsCount].loggedIn = 0;
        clients[clientsCount].loginStage = 0;
        clients[clientsCount].inGame = 0;
        clients[clientsCount].ready = 0;
        clients[clientsCount].opponent = -1;
        clients[clientsCount].privateMode = 0;
        clients[clientsCount].name[0] = '\0';
        clientsCount++;
        FD_SET(newfd, &master_set);
        if (newfd > max_fd) max_fd = newfd;
        send(newfd, "Entrez votre pseudo :\n", strlen("Entrez votre pseudo :\n"), 0);
        return;
    }
    send(newfd, "Le serveur est surchargé, réessayez plus tard!\n", strlen("Le serveur est surchargé, réessayez plus tard!\n"), 0);
    close(newfd);
}

void handleClientMessage(int fd) {
    char buf[BUF_SIZE];
    int n = recv(fd, buf, sizeof(buf)-1, 0);
    if (n <= 0) { removeClient(fd); return; }
    buf[n] = 0;

    int i;
    for (i = 0; i < MAX_CLIENTS; i++) if (clients[i].fd == fd) break;
    if (i == MAX_CLIENTS) return;

    buf[strcspn(buf, "\r\n")] = 0;


    if (!clients[i].loggedIn) {
      if (clients[i].loginStage == 0) {
          if (strlen(buf) == 0) { send(fd, "Entrez votre pseudo:\n", strlen("Entrez votre pseudo:\n"), 0); return; }

          size_t uname_len = strlen(buf);
          if (uname_len > 14) uname_len = 14;
          memcpy(clients[i].name, buf, uname_len);
          clients[i].name[uname_len] = '\0';
          int idx = findAccount(clients[i].name);
          if (idx >= 0)
          {
            if(accounts[idx].isLogged!=1) {
              char msg[80];
              snprintf(msg, sizeof(msg), "Ravi de vous revoir, %s!\nEntrez votre mot de passe:\n", clients[i].name);
              send(fd, msg, strlen(msg), 0);
              accounts[idx].isLogged=1;
            clients[i].loginStage = 1;
          } 
          else
          {
            char msg[80];
            snprintf(msg, sizeof(msg), "%s est déjà connecté\nEntrez votre pseudo :\n", clients[i].name);
            send(fd, msg, strlen(msg), 0);
            return;
          }
        }
          else {
              char msg[80];
              snprintf(msg, sizeof(msg), "Bienvenue, %s! Veuillez créer un mot de passe:\n", clients[i].name);
              send(fd, msg, strlen(msg), 0);
              accounts[idx].isLogged=1;
          clients[i].loginStage = 1;
          }
          
          return;
      } else {
          int idx = findAccount(clients[i].name);
          if (idx >= 0) {
              if (strcmp(accounts[idx].password, buf) == 0) {
                  clients[i].loggedIn = 1;
                  clients[i].loginStage = 2;
                  accounts[idx].isLogged=1;
                  send(fd, "Vous êtes connectés avec succès !\n", strlen("Vous êtes connectés avec succès !\n"), 0);
              } else {
                  send(fd, "ERREUR : Le mot de passe est incorrect.\nEntrez votre pseudo :\n",
                       strlen("ERREUR : Le mot de passe est incorrect.\nEntrez votre pseudo :\n"), 0);
                  clients[i].loginStage = 0;
              }
          } else {
              if (accountCount < MAX_ACCOUNTS) {
                  memcpy(accounts[accountCount].username, clients[i].name, 15);
                  accounts[accountCount].username[15] = '\0';

                  size_t pwd_len = strlen(buf);
                  if (pwd_len > 30) pwd_len = 30;
                  memcpy(accounts[accountCount].password, buf, pwd_len);
                  accounts[accountCount].password[pwd_len] = '\0';
                  accounts[accountCount].bio[0] = '\0';
                  accounts[accountCount].friends[0] = '\0';
                  accountCount++; saveAccounts();
                  clients[i].loggedIn = 1; clients[i].loginStage = 2;
                  accounts[accountCount].isLogged=1;
                  send(fd, "Votre nouveau compte à été crée avec succès.\n",
                       strlen("Votre nouveau compte à été crée avec succès.\n"), 0);
              } else {
                  send(fd, "ERREUR : La base de donnée de stockage des comptes est pleine.\n",
                       strlen("ERREUR : La base de donnée de stockage des comptes est pleine.\n"), 0);
                  clients[i].loginStage = 0;
              }
          }
          return;
      }
  }
  if (strncmp(buf, "LIST", 4) == 0) {
      char list[512] = "SONT CONNECTES ET DISPONIBLES:";
      for (int j = 0; j < MAX_CLIENTS; j++)
          if (clients[j].fd != -1 && clients[j].loggedIn && j != i && !clients[j].inGame) {
              strcat(list, " "); strcat(list, clients[j].name);
          }
      if (strcmp(list, "SONT CONNECTES ET DISPONIBLES:") == 0) strcat(list, " (aucun autre joueur)");
      strcat(list, "\n"); send(fd, list, strlen(list), 0);
  }
  else if (strncmp(buf, "LIST_GAMES", 10) == 0 || strncmp(buf, "GAMES", 5) == 0) {
      char msg[512] = "PARTIES EN COURS :\n"; int c = 0;
      for (int g = 0; g < MAX_GAMES; g++) if (games[g].active) {
          char line[128];
          snprintf(line, sizeof(line), "  ID %d: %s vs %s\n", g, games[g].j0.pseudo, games[g].j1.pseudo);
          strcat(msg, line); c++;
      }
      if (!c) strcat(msg, "  (aucune partie en cours)\n");
      send(fd, msg, strlen(msg), 0);
  }
  else if (strncmp(buf, "OBSERVE ", 8) == 0) {
      int id; if (sscanf(buf+8, "%d", &id) != 1) { send(fd, "ERREUR Usage: OBSERVE <id>\n", strlen("ERREUR Usage: OBSERVE <id>\n"), 0); return; }
      if (id < 0 || id >= MAX_GAMES || !games[id].active) { send(fd, "ERREUR : ID de partie incorrect\n", strlen("ERREUR : ID de partie incorrect\n"), 0); return; }
      Game *g = &games[id];
      if (!canObserve(g, clients[i].name)) {
          send(fd, "ERREUR : La partie est privée et ne peut donc pas être observée.\n",
               strlen("ERREUR : La partie est privée et ne peut donc pas être observée.\n"), 0);
          return;
      }
      if (g->observerCount < MAX_CLIENTS) {
          g->observers[g->observerCount++] = fd;
          char ok[128];
          snprintf(ok, sizeof(ok), "Vous observez la partie : %d: %s vs %s\n", id, g->j0.pseudo, g->j1.pseudo);
          send(fd, ok, strlen(ok), 0);
          sendBoard(g);
      } else {
          send(fd, "ERREUR : Trop de personnes observent cette partie.\n",
               strlen("ERREUR : Trop de personnes observent cette partie.\n"), 0);
      }
  }
  else if (strncmp(buf, "OUT_OBSERVER", 12) == 0) {
      for (int g = 0; g < MAX_GAMES; g++) {
          for (int z = 0; z < games[g].observerCount; z++) {
              if (games[g].observers[z] == fd) {
                  games[g].observers[z] = games[g].observers[--games[g].observerCount];
                  send(fd, "Vous avez arrêté d'observer cette partie soporiphique et vous voilà à nouveau dans le menu principal !\n",
                       strlen("Vous avez arrêté d'observer cette partie soporiphique et vous voilà à nouveau dans le menu principal !\n"), 0);
                  return;
              }
          }
      }
      send(fd, "ERREUR : Vous n'êtes pas en train d'observer une partie.\n",
           strlen("ERREUR : Vous n'êtes pas en train d'observer une partie.\n"), 0);
  }
  else if (strncmp(buf, "PRIVATE ", 8) == 0) {
      char mode[8] = {0}; sscanf(buf+8, "%7s", mode);
      if (strcasecmp(mode, "ON") == 0) {
          clients[i].privateMode = 1;
          send(fd, "Le mode privé est activé : vos parties sont uniquement observables par vos amis.\n",
               strlen("Le mode privé est activé : vos parties sont uniquement observables par vos amis.\n"), 0);
      } else {
          clients[i].privateMode = 0;
          send(fd, "Le mode privé est désactivé : vos parties sont observables par tous.\n",
               strlen("Le mode privé est désactivé : vos parties sont observables par tous.\n"), 0);
      }
  }
    else if (strncmp(buf, "FRIEND ", 7) == 0) {
        char target[16]; if (sscanf(buf+7, "%15s", target) != 1) { send(fd, "ERREUR Usage: FRIEND <user>\n", strlen("ERREUR Usage: FRIEND <user>\n"), 0); return; }
        int me = findAccount(clients[i].name), you = findAccount(target);
        if (me < 0 || you < 0) { send(fd, "ERREUR Utilisateur inconnu\n", strlen("ERREUR Utilisateur inconnu\n"), 0); return; }
        if (isFriend(&accounts[me], target)) { send(fd, "Vous êtes déjà ami!\n", strlen("Vous êtes déjà ami!\n"), 0); return; }

        size_t current_len = strlen(accounts[me].friends);
        size_t target_len = strlen(target);
        if (current_len + target_len + 2 >= sizeof(accounts[me].friends)) {
            send(fd, "ERREUR : Vous avez déjà trop d'amis!\n", strlen("ERREUR : Vous avez déjà trop d'amis!\n"), 0);
            return;
        }
        if (current_len > 0) strncat(accounts[me].friends, ",", sizeof(accounts[me].friends) - current_len - 1);
        strncat(accounts[me].friends, target, sizeof(accounts[me].friends) - strlen(accounts[me].friends) - 1);
        saveAccounts();
        send(fd, "Ami ajouté avec succès.\n", strlen("Ami ajouté avec succès.\n"), 0);
    }
    else if (strncmp(buf, "UNFRIEND ", 9) == 0) {
        char target[16]; if (sscanf(buf+9, "%15s", target) != 1) { send(fd, "ERREUR Usage: UNFRIEND <user>\n", strlen("ERREUR Usage: UNFRIEND <user>\n"), 0); return; }
        int me = findAccount(clients[i].name);
        if (me < 0) { send(fd, "ERREUR : Compte non trouvé.\n", strlen("ERREUR : Compte non trouvé.\n"), 0); return; }
        char tmp[256];
        tmp[0] = '\0';
        char list[256];
        strncpy(list, accounts[me].friends, sizeof(list) - 1);
        list[sizeof(list) - 1] = '\0';
        char *save_tok = NULL;
        char *tok = strtok_r(list, ",", &save_tok);
        int first = 1, removed = 0;
        while (tok) {
            if (strcmp(tok, target) != 0) {
                size_t tmp_len = strlen(tmp);
                if (!first && tmp_len + 1 < sizeof(tmp)) {
                    strncat(tmp, ",", sizeof(tmp) - tmp_len - 1);
                    tmp_len++;
                }
                if (tmp_len + strlen(tok) < sizeof(tmp)) {
                    strncat(tmp, tok, sizeof(tmp) - tmp_len - 1);
                }
                first = 0;
            } else removed = 1;
            tok = strtok_r(NULL, ",", &save_tok);
        }

        size_t tmp_len = strlen(tmp);
        if (tmp_len > 255) tmp_len = 255;
        memcpy(accounts[me].friends, tmp, tmp_len);
        accounts[me].friends[tmp_len] = '\0';
        saveAccounts();
        send(fd, removed ? "Ami retiré avec succès.\n" : "Vous n'êtes pas ami avec ce joueur.\n", removed ? strlen("Ami retiré avec succès.\n") : strlen("Vous n'êtes pas ami avec ce joueur.\n"), 0);
    }
    else if (strncmp(buf, "BIO ", 4) == 0) {
        int idx = findAccount(clients[i].name);
        if (idx >= 0) {

            int lines = 0; char cleaned[512]; size_t k = 0;
            size_t remaining = strlen(buf) - 4;
            if (remaining > 510) remaining = 510;
            for (size_t p = 4; p < 4 + remaining && buf[p] && k+1 < sizeof(cleaned); p++) {
                char c = buf[p];
                if (c == '\n' || c == '\r') continue;
                if (c == '\t') c = ' ';
                if (c == '|') c = '/';
                if (c == '\0') break;
                if (c == '\\' && buf[p+1]=='n') { c = '\n'; p++; }
                if (c == '\n') { if (++lines >= 10) { c=' '; } }
                cleaned[k++] = c;
            }
            cleaned[k] = '\0';

            size_t bio_len = strlen(cleaned);
            if (bio_len > 510) bio_len = 510;
            memcpy(accounts[idx].bio, cleaned, bio_len);
            accounts[idx].bio[bio_len] = '\0';
            saveAccounts();
            send(fd, "Biographie mise à jour avec succès. \n", strlen("Biographie mise à jour avec succès. \n"), 0);
        } else send(fd, "ERREUR : Compte non trouvé.\n", strlen("ERREUR : Compte non trouvé.\n"), 0);
    }
    else if (strncmp(buf, "SHOWBIO ", 8) == 0) {
        char target[16]; if (sscanf(buf+8, "%15s", target) != 1) { send(fd, "ERREUR Usage: SHOWBIO <user>\n", strlen("ERREUR Usage: SHOWBIO <user>\n"), 0); return; }
        int idx = findAccount(target);
        if (idx >= 0) {
            char msg[700];
            snprintf(msg, sizeof(msg), "\n--- BIOGRAPHIE de %s ---\n%s\n-----------------\n",
                     accounts[idx].username,
                     accounts[idx].bio[0] ? accounts[idx].bio : "(aucune biographie)");
            send(fd, msg, strlen(msg), 0);
        } else send(fd, "ERREUR : Compte non trouvé.\n", strlen("ERREUR : Compte non trouvé.\n"), 0);
    }
    else if (strncmp(buf, "CHALLENGE ", 10) == 0) {
        char target[16]; if (sscanf(buf+10, "%15s", target) != 1) { 
            
            send(fd, "ERR Usage: CHALLENGE <user>\n", strlen("ERR Usage: CHALLENGE <user>\n"), 0); 
            return;
         }
        if (strcmp(target, clients[i].name) == 0) { send(fd, "ERREUR : Vous ne pouvez pas vous défier vous-même !\n", strlen("ERREUR : Vous ne pouvez pas vous défier vous-même !\n"), 0); return; }
        else {
            int idxG=findGameByPlayer(target);
            if(idxG!=-1)
            {
                send(fd,"ERREUR : Le joueur demandé est déjà dans une partie .\n",strlen("ERREUR : Le joueur demandé est déjà dans une partie .\n"),0); return;
            }
        }
        for (int j = 0; j < MAX_CLIENTS; j++)
            if (clients[j].fd != -1 && clients[j].loggedIn && strcmp(clients[j].name, target)==0) {
              char msg[128];
              snprintf(msg, sizeof(msg),
                     "%s vous a défié!\n"
                     "Pour accepter, tapez : ACCEPT %s\n"
                     "Pour refuser, tapez : REFUSE %s\n",
                     clients[i].name, clients[i].name, clients[i].name);

              send(clients[j].fd, msg, strlen(msg), 0);
              send(fd, "Le défi a été envoyé avec succès\n", strlen("Le défi a été envoyé avec succès\n"), 0); return;
            }
        send(fd, "ERREUR : Cet utilisateur n'existe pas.\n", strlen("ERREUR : Cet utilisateur n'existe pas.\n"), 0);
    }
    else if (strncmp(buf, "REFUSE ", 7) == 0) {
        char opp[16]; if (sscanf(buf+7, "%15s", opp) != 1) { send(fd, "ERREUR Usage: REFUSE <user>\n", strlen("ERREUR Usage: REFUSE <user>\n"), 0); return; }
        for (int j = 0; j < MAX_CLIENTS; j++)
            if (clients[j].fd != -1 && strcmp(clients[j].name, opp)==0) {
                char msg[64]; snprintf(msg, sizeof(msg), "%s a décliné votre défi\n", clients[i].name);
                send(clients[j].fd, msg, strlen(msg), 0);
                send(fd, "Vous avez décliné ce défi.\n", strlen("Vous avez décliné ce défi.\n"), 0); return;
            }
        send(fd, "ERREUR : Cet utilisateur n'existe pas.\n", strlen("ERREUR : Cet utilisateur n'existe pas.\n"), 0);
    }
    else if (strncmp(buf, "ACCEPT ", 7) == 0) {
        char opp[16]; if (sscanf(buf+7, "%15s", opp) != 1) { send(fd, "ERREUR Usage: ACCEPT <user>\n", strlen("ERREUR Usage: ACCEPT <user>\n"), 0); return; }
        for (int j = 0; j < MAX_CLIENTS; j++)
            if (clients[j].fd != -1 && strcmp(clients[j].name, opp)==0) {
                startGame(i, j); return;
            }
        send(fd, "ERREUR : Cet utilisateur n'existe pas.\n", strlen("ERREUR : Cet utilisateur n'existe pas.\n"), 0);
    }
    else if (strncmp(buf, "READY", 5) == 0) {
        if (!clients[i].inGame) { send(fd, "ERREUR : Pas en partie.\n", strlen("ERREUR : Pas en partie.\n"), 0); return; }
        clients[i].ready = 1;
        int j = clients[i].opponent;
        if (j >= 0 && clients[j].ready) {
            for (int k = 0; k < MAX_GAMES; k++){
                if (games[k].active && (
                    strcmp(games[k].j0.pseudo, clients[i].name)==0 ||
                    strcmp(games[k].j1.pseudo, clients[i].name)==0))
                    sendBoard(&games[k]);
                  }
        }
    }
    else if (strncmp(buf, "MOVE ", 5) == 0) {
        int pit; if (sscanf(buf+5, "%d", &pit) != 1) { send(fd, "ERREUR Usage: MOVE <0-11>\n", strlen("ERREUR Usage: MOVE <0-11>\n"), 0); return; }
        processMove(i, pit);
    }
    else if (strncmp(buf, "CANCEL_GAME", 11) == 0) {
        if (!clients[i].inGame) { send(fd, "ERREUR : Aucune partie est en cours.\n", strlen("ERREUR : Aucune partie est en cours.\n"), 0); return; }
        int opp = clients[i].opponent;
        if (opp >= 0 && clients[opp].fd != -1) {
            char msg[64]; snprintf(msg, sizeof(msg), "INFO %s a abandonné la partie.\n", clients[i].name);
            send(clients[opp].fd, msg, strlen(msg), 0);
            clients[opp].inGame = 0; clients[opp].opponent = -1; clients[opp].ready = 0;
        }
        for (int g = 0; g < MAX_GAMES; g++)
            if (games[g].active && (
                strcmp(games[g].j0.pseudo, clients[i].name)==0 ||
                strcmp(games[g].j1.pseudo, clients[i].name)==0))
                games[g].active = 0;
        clients[i].inGame = 0; clients[i].opponent = -1; clients[i].ready = 0;
        send(fd, "Vous avez abandonné la partie et êtes de retour au menu.\n", strlen("Vous avez abandonné la partie et êtes de retour au menu.\n"), 0);
    }
    else if (strncmp(buf, "MESSAGE ", 8) == 0) {
        char target[16], msgBody[480];
        if (sscanf(buf+8, "%15s %479[^\n]", target, msgBody) < 2) { send(fd, "ERREUR Usage: MESSAGE <user> <message>\n", strlen("ERREUR Usage: MESSAGE <user> <message>\n"), 0); return; }
        for (int j = 0; j < MAX_CLIENTS; j++)
            if (clients[j].fd != -1 && clients[j].loggedIn &&
                strcmp(clients[j].name, target)==0) {
                char pm[512];
                snprintf(pm, sizeof(pm), "Message privé de %s: %s\n", clients[i].name, msgBody);
                pm[sizeof(pm) - 1] = '\0';
                send(clients[j].fd, pm, strlen(pm), 0);
                send(fd, "Message envoyé!\n", strlen("Message envoyé!\n"), 0); return;
            }
        send(fd, "ERREUR : Utilisateur non trouvé.\n", strlen("ERREUR : Utilisateur non trouvé.\n"), 0);
    }
    else if (strncmp(buf, "SAY ", 4) == 0) {
        char msg[512];
        size_t msglen = strlen(buf + 4);
        if (msglen > 450) msglen = 450;
        snprintf(msg, sizeof(msg), "CHAT %s: %.*s\n", clients[i].name, (int)msglen, buf+4);
        msg[sizeof(msg) - 1] = '\0';
        broadcast(msg, fd);
    }
    else if (strncmp(buf, "QUIT", 4) == 0) {
        int idx = findAccount(clients[i].name);
        accounts[idx].isLogged=0;
        removeClient(fd);
    }
    else {
        send(fd, "ERREUR : Commande inconnue.\n", strlen("ERREUR : Commande inconnue.\n"), 0);
    }
}

int createServerSocket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); exit(1);
    }
    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); exit(1);
    }
    return server_fd;
}

void runServerLoop(int server_fd) {
    while (1) {
        fd_set read_fds = master_set;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) { perror("select"); break; }
        for (int i = 0; i <= max_fd; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) handleNewConnection(server_fd);
                else handleClientMessage(i);
            }
        }
    }
}

