CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
INCLUDE  = -Isrc -Istarting-kit
LDLIBS   = -lm
BIN      = bin

.PHONY: all clean

all: $(BIN)/taquin_vs $(BIN)/taquin_baseline

$(BIN):
	mkdir -p $(BIN)

# Mode interactif : le joueur affronte A*.
$(BIN)/taquin_vs: src/taquin_vs.c src/board.c starting-kit/list.c | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

# Solveur BFS / A* d'origine, restauré tel quel (commit c34e2a1).
# Sert de référence historique : c'est le « avant » des phases 1 et 2.
$(BIN)/taquin_baseline: src/baseline.c src/board.c starting-kit/list.c | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(BIN)
