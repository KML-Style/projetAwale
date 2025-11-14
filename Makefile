CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
LDFLAGS =
OBJDIR  = obj
BINDIR  = bin

# Source files
SRV_SRC = serveur.c jeu.c compte.c var.c gestionClient.c gestionPartie.c
CLI_SRC = client.c
SRV_OBJ = $(SRV_SRC:%.c=$(OBJDIR)/%.o)
CLI_OBJ = $(CLI_SRC:%.c=$(OBJDIR)/%.o)

# Binaries
SERVER  = $(BINDIR)/serveur
CLIENT  = $(BINDIR)/client

# Default target
all: $(SERVER) $(CLIENT)

# Build directories if missing
$(OBJDIR) $(BINDIR):
	mkdir -p $(OBJDIR) $(BINDIR)

# Compile object files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link server
$(SERVER): $(SRV_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Link client
$(CLIENT): $(CLI_OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Convenience run commands
run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 8080

.PHONY: all clean run-server run-client

