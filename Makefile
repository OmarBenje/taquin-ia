CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -O2
INCLUDE   = -Isrc -Istarting-kit -Itests
LDLIBS    = -lm
DEPFLAGS  = -MMD -MP

BIN       = bin
OBJ       = $(BIN)/obj
OBJ15     = $(BIN)/obj15

# Cœur du jeu, partagé par tous les binaires.
CORE      = src/puzzle.c src/heuristic.c src/enumerate.c
# Adaptateur vers la structure Item du squelette fourni.
KIT       = src/board.c starting-kit/list.c
# Solveur instrumenté.
SEARCH    = src/search.c src/stats.c src/node.c src/pqueue.c src/hashset.c

LIB       = $(CORE) $(KIT) $(SEARCH)
LIB_O     = $(patsubst %.c,$(OBJ)/%.o,$(LIB))
LIB_O15   = $(patsubst %.c,$(OBJ15)/%.o,$(LIB))

BINARIES  = $(BIN)/taquin $(BIN)/taquin15 $(BIN)/taquin_vs $(BIN)/taquin_baseline
TESTS     = $(BIN)/test_puzzle $(BIN)/test_heuristic $(BIN)/test_pqueue \
            $(BIN)/test_hashset $(BIN)/test_search $(BIN)/test_enumerate

.PHONY: all help demo test bench report clean FORCE
.DEFAULT_GOAL := all

help:
	@echo "make          compile les binaires                   (~2 s)"
	@echo "make test     la suite de tests                      (~4 s)"
	@echo "make demo     20 instances resolues et verifiees     (~1 s)"
	@echo "make bench    regenere results/*.csv                 (long)"
	@echo "make report   regenere docs/BENCHMARKS.md et les graphes"
	@echo "make clean    supprime bin/"

all: $(BINARIES)
	@echo "-> bin/ pret. 'make demo' pour un resultat, 'make help' pour le reste."

demo: $(BIN)/taquin
	./$(BIN)/taquin --runs 20 --verify

# ------------------------------------------------------------------ #
# Témoin de configuration                                             #
#                                                                     #
# Sans lui, `make CFLAGS=-O0` ne recompilait rien : make ne compare    #
# que les dates, pas les options. On mesurait donc un binaire -O2 en   #
# croyant mesurer du -O0. Le fichier n'est réécrit que si la ligne     #
# change, sinon sa date ne bouge pas et rien n'est reconstruit.        #
# ------------------------------------------------------------------ #
$(BIN)/.flags: FORCE
	@mkdir -p $(BIN)
	@echo '$(CC) $(CFLAGS)' | cmp -s - $@ 2>/dev/null || echo '$(CC) $(CFLAGS)' > $@
FORCE:

# -MMD -MP génère un .d par objet : modifier un en-tête recompile
# désormais ce qui en dépend. Auparavant, toucher src/stats.h ne
# recompilait rien du tout — on débuggait un binaire périmé.
$(OBJ)/%.o: %.c $(BIN)/.flags
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ15)/%.o: %.c $(BIN)/.flags
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -DWH_BOARD=4 $(INCLUDE) -c $< -o $@

# ------------------------------------------------------------------ #
# Binaires                                                            #
# ------------------------------------------------------------------ #

# Banc de mesure : c'est lui qui produit les CSV du README.
$(BIN)/taquin: $(OBJ)/src/taquin.o $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Même code en 4x4 : le 15-puzzle, là où A* meurt en mémoire.
$(BIN)/taquin15: $(OBJ15)/src/taquin.o $(LIB_O15)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Mode interactif : le joueur affronte A*.
$(BIN)/taquin_vs: $(OBJ)/src/taquin_vs.o $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Solveur BFS / A* d'origine, restauré depuis le commit c34e2a1 (seule la
# graine a été rendue explicite). Référence historique : le « avant ».
$(BIN)/taquin_baseline: $(OBJ)/src/baseline.o $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# ------------------------------------------------------------------ #
# Tests                                                               #
# ------------------------------------------------------------------ #

$(BIN)/test_puzzle:    $(OBJ)/tests/test_puzzle.o    $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_heuristic: $(OBJ)/tests/test_heuristic.o $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_pqueue:    $(OBJ)/tests/test_pqueue.o    $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_hashset:   $(OBJ)/tests/test_hashset.o   $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_search:    $(OBJ)/tests/test_search.o    $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
$(BIN)/test_enumerate: $(OBJ)/tests/test_enumerate.o $(LIB_O)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

test: $(TESTS)
	@fail=0; for t in $(TESTS); do ./$$t || fail=1; done; \
	 if [ $$fail -ne 0 ]; then echo "TESTS EN ECHEC"; exit 1; \
	 else echo "tous les tests passent"; fi

clean:
	rm -rf $(BIN)

-include $(LIB_O:.o=.d) $(LIB_O15:.o=.d)
-include $(OBJ)/src/taquin.d $(OBJ15)/src/taquin.d
-include $(OBJ)/src/taquin_vs.d $(OBJ)/src/baseline.d
-include $(OBJ)/tests/test_puzzle.d $(OBJ)/tests/test_heuristic.d
-include $(OBJ)/tests/test_pqueue.d $(OBJ)/tests/test_hashset.d
-include $(OBJ)/tests/test_search.d $(OBJ)/tests/test_enumerate.d

# Ajoute aussi l'aide sur les options d'analyse.
