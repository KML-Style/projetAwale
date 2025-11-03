#include "player.h"

void setUsername(char* user, player* p){
    strncpy(p->username, user, sizeof p->username - 1);
    p->username[sizeof p->username - 1] = '\0';

} 