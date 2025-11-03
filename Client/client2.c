#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "client2.h"
#include "../jeu.h"

static void init(void);
static void end(void);
static void app(const char *address, const char *pseudo);
static int init_connection(const char *address);
static void end_connection(int sock);

void handle_input(SOCKET sock, char *input)
{
    if (strlen(input) == 0) return;
    write_server(sock, input);
}

bool is_command(const char *input)
{
    return input[0] == '/';
}

void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) < 0)
    {
        puts("WSAStartup failed!");
        exit(EXIT_FAILURE);
    }
#endif
}

void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

static void app(const char *address, const char *pseudo)
{
    SOCKET sock = init_connection(address);
    char buffer[BUF_SIZE];

    fd_set rdfs;
    Jeu jeu;
    initPlateau(&jeu);

    write_server(sock, pseudo);

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (!fgets(buffer, BUF_SIZE, stdin))
                continue;

            char *p = strchr(buffer, '\n');
            if (p) *p = 0;

            handle_input(sock, buffer);
        }

        if (FD_ISSET(sock, &rdfs))
        {
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("Server disconnected!\n");
                break;
            }
            puts(buffer);
        }
    }

    end_connection(sock);
}

static int init_connection(const char *address)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo;

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    hostinfo = gethostbyname(address);
    if (!hostinfo)
    {
        fprintf(stderr, "Unknown host %s\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR*)hostinfo->h_addr;
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (connect(sock, (SOCKADDR*)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()");
        exit(errno);
    }

    return sock;
}

static void end_connection(int sock)
{
    closesocket(sock);
}

int read_server(SOCKET sock, char *buffer)
{
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0)
    {
        perror("recv()");
        exit(errno);
    }
    buffer[n] = 0;
    return n;
}

void write_server(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: %s [address] [pseudo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init();
    app(argv[1], argv[2]);
    end();
    return EXIT_SUCCESS;
}
