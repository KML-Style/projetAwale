#include "../client/player.h"
#include "server2.h"
#include "client2.h"


typedef struct
{
    player* playerList;
    int nbPlayer;
    int board[12];
    int currentPlayer;
}awale;


void printAllPlayer(SOCKET sock,player* plist,int nbPlayer);
void saveAllPlayer(Client* clients,int actual,player* playerList, int* nbPlayer);

static void handle_client_command(Client* clients,int actual ,int nbPlayer,Client client,player* playerList, const char *buffer);