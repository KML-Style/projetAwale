#include "compte.h"
#include <stdio.h>
#include <string.h>

static void bio_encode(const char *in, char *out, size_t outsz) {
    size_t k = 0;
    for (size_t i = 0; in[i] && k+1 < outsz; i++) {
        char c = in[i];
        if (c == '\n') c = '|';
        if (c == ':') c = ' ';
        out[k++] = c;
    }
    out[k] = '\0';
}

static void bio_decode(const char *in, char *out, size_t outsz) {
    size_t k = 0;
    for (size_t i = 0; in[i] && k+1 < outsz; i++) {
        char c = in[i] == '|' ? '\n' : in[i];
        out[k++] = c;
    }
    out[k] = '\0';
}

int findAccount(const char *username) {
    for (int i = 0; i < accountCount; i++)
        if (strcmp(accounts[i].username, username) == 0) return i;
    return -1;
}

int isFriend(Account *acc, const char *username) {
    if (!acc || !username || !*username) return 0;
    if (acc->friends[0] == '\0') return 0;
    char tmp[256]; strncpy(tmp, acc->friends, sizeof(tmp)-1); tmp[255]='\0';
    char *tok = strtok(tmp, ",");
    while (tok) {
        if (strcmp(tok, username) == 0) return 1;
        tok = strtok(NULL, ",");
    }
    return 0;
}

int loadAccounts(void) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;
    char line[1024];
    int n = 0;
    while (n < MAX_ACCOUNTS && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == 0) continue;

        char *p1 = strchr(line, ':'); if (!p1) continue; *p1++ = 0;
        char *p2 = strchr(p1,   ':'); if (!p2) continue; *p2++ = 0;
        char *p3 = strchr(p2,   ':');
        char *u = line, *pw = p1, *bio_enc = NULL, *friends = NULL;

        if (p3) { *p3++ = 0; bio_enc = p2; friends = p3; }
        else    { bio_enc = p2; friends = ""; }

        size_t u_len = strlen(u); if (u_len > 15) u_len = 15;
        memcpy(accounts[n].username, u, u_len); accounts[n].username[u_len] = '\0';
        size_t pw_len = strlen(pw); if (pw_len > 31) pw_len = 31;
        memcpy(accounts[n].password, pw, pw_len); accounts[n].password[pw_len] = '\0';
        bio_decode(bio_enc, accounts[n].bio, sizeof(accounts[n].bio));
        strncpy(accounts[n].friends, friends ? friends : "", sizeof(accounts[n].friends)-1);
        accounts[n].friends[sizeof(accounts[n].friends)-1] = '\0';
        n++;
    }
    fclose(f);
    return n;
}

void saveAccounts(void) {
    FILE *f = fopen(USERS_FILE, "w");
    if (!f) return;
    for (int i = 0; i < accountCount; i++) {
        char bio_oneline[600];
        bio_encode(accounts[i].bio, bio_oneline, sizeof(bio_oneline));
        fprintf(f, "%s:%s:%s:%s\n",
                accounts[i].username,
                accounts[i].password,
                bio_oneline,
                accounts[i].friends);
    }
    fclose(f);
}

