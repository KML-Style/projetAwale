#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "multiplayer.h"

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
awale _awale;

static void app(void)
{
   SOCKET sock = init_connection();
   
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   

   fd_set rdfs;

   while(1)
   {
      player* playerList=malloc(MAX_CLIENTS*sizeof(player));
      int i,nbPlayer = 0;
      FD_ZERO(&rdfs);
      

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         write_client(csock,"Bienvenue sur Awale Game\n");


         
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

        
         

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         c.state=WAITING_NAME;
         c.name[0]="\0";
         clients[actual] = c;
         actual++;
          write_client(csock,"Entrez un nom\n");
         
         
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client* client = &clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, *client, actual, buffer, 1);
               }
               
               if(client->state==WAITING_NAME){
                buffer[strcspn(buffer, "\r\n")] = 0;
                 strncpy(client->name, buffer, BUF_SIZE - 1);

                 client->state=CONNECTED;

                 max = client->sock > max ? client->sock : max;

                 char welcome[BUF_SIZE];
                  snprintf(welcome, sizeof welcome, "Bienvenue %s\n", client->name);
                  write_client(client->sock, welcome);

               } 
               else
               {

                  handle_client_command(clients,actual,nbPlayer,*client,playerList,buffer);
                  
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void handle_client_command(Client* clients,int actual,int nbPlayer,Client client,player* playerList,const char *buffer)
{
   if(strcasecmp(buffer,"Liste")==0)
   {
      saveAllPlayer(clients,actual,playerList,&nbPlayer);
      printAllPlayer(client.sock,playerList,nbPlayer);
   }
   else{
      send_message_to_all_clients(clients, client, actual, buffer, 0);
   }
}

void printAllPlayer(SOCKET sock,player* pList,int nbPlayer)
{
    for (int i=0;i<nbPlayer;i++){

      char buffer[BUF_SIZE];
      
      snprintf(buffer,sizeof(buffer),"- %s \n",pList[i].username);
      write_client(sock,buffer);
    }
}

void saveAllPlayer(Client* clients,int actual,player* playerList,int* nbPlayer)
{
   int idx=0;
   for(int i=0;i<actual;i++)
   {
      if(clients[i].state==CONNECTED)
      {
         playerList[idx].id=clients[i].sock;
         //*playerList[idx].username=clients[i].name;
         strncpy(playerList[idx].username, clients[i].name, sizeof playerList[idx].username - 1);
         playerList[idx].username[sizeof playerList[idx].username - 1] = '\0';

         //setUsername(clients[i].name,&playerList[idx]);
         idx++;
      }
   }
   _awale.playerList=playerList;
   *nbPlayer=idx;

}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
