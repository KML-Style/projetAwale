CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS =
OBJDIR  = obj
BINDIR  = bin

SRV_SRC = serveur.c jeu.c compte.c var.c gestionClient.c gestionPartie.c
CLI_SRC = client.c
SRV_OBJ = $(SRV_SRC:%.c=$(OBJDIR)/%.o)
CLI_OBJ = $(CLI_SRC:%.c=$(OBJDIR)/%.o)

SERVER  = $(BINDIR)/serveur
CLIENT  = $(BINDIR)/client

all: $(SERVER) $(CLIENT)

$(OBJDIR) $(BINDIR):
	mkdir -p $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER): $(SRV_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(CLIENT): $(CLI_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)



clean:
	rm -rf $(OBJDIR) $(BINDIR)

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 8080

.PHONY: all clean run-server run-client

