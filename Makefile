CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
INCLUDE  = -Isrc -Istarting-kit -Itests
LDLIBS   = -lm
BIN      = bin

# Cœur du jeu, partagé par tous les binaires.
CORE     = src/puzzle.c src/heuristic.c
# Adaptateur vers la structure Item du squelette fourni.
KIT      = src/board.c starting-kit/list.c

TESTS    = $(BIN)/test_puzzle $(BIN)/test_heuristic

.PHONY: all test clean

all: $(BIN)/taquin_vs $(BIN)/taquin_baseline

$(BIN):
	@mkdir -p $(BIN)

# Mode interactif : le joueur affronte A*.
$(BIN)/taquin_vs: src/taquin_vs.c $(KIT) $(CORE) | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

# Solveur BFS / A* d'origine, restauré depuis le commit c34e2a1 (seule la
# graine a été rendue explicite). Référence historique : le « avant ».
$(BIN)/taquin_baseline: src/baseline.c $(KIT) $(CORE) | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

$(BIN)/test_puzzle: tests/test_puzzle.c $(CORE) | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

$(BIN)/test_heuristic: tests/test_heuristic.c $(CORE) | $(BIN)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LDLIBS)

test: $(TESTS)
	@fail=0; for t in $(TESTS); do ./$$t || fail=1; done; \
	 if [ $$fail -ne 0 ]; then echo "TESTS EN ECHEC"; exit 1; \
	 else echo "tous les tests passent"; fi

clean:
	rm -rf $(BIN)
