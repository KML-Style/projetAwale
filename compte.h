#ifndef COMPTE_H
#define COMPTE_H

#include "var.h"

int findAccount(const char *username);
int isFriend(Account *acc, const char *username);
int loadAccounts(void);
void saveAccounts(void);

#endif

