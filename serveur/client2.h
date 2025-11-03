#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
typedef enum {
    WAITING_NAME,
    CONNECTED
} ClientState;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   ClientState state
}Client;

#endif /* guard */
