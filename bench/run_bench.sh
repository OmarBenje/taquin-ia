#!/usr/bin/env bash
#
# Produit tous les CSV du README.
#
#   ./bench/run_bench.sh              # complet
#   SCALE=20 ./bench/run_bench.sh     # 20x plus court, memes graines
#   SEED=7   ./bench/run_bench.sh     # autre tirage
#
# Deterministe par ses valeurs par DEFAUT, pas par l'absence d'options :
# retirer les options forcerait a editer le script, donc a diverger de la
# version commitee.
#
# Ecriture atomique : tout va dans results/.new/ puis bascule d'un bloc. Une
# interruption ne laisse jamais un results/ moitie neuf moitie ancien.
set -euo pipefail
cd "$(dirname "$0")/.."

SEED=${SEED:-1}
SCALE=${SCALE:-1}
OUT=results
NEW="$OUT/.new"

# Deux plafonds, parce que les deux algorithmes meurent de causes differentes.
ASTAR_BYTES=${ASTAR_BYTES:-536870912}      # 512 Mo
IDA_NODES=${IDA_NODES:-4000000000}         # 4e9 noeuds

n() { echo $(( $1 / SCALE > 0 ? $1 / SCALE : 1 )); }

trap 'rm -rf "$NEW"' EXIT
rm -rf "$NEW"; mkdir -p "$NEW"

make all >/dev/null

# --- environnement ---------------------------------------------------------
# Les flags REELLEMENT utilises viennent du temoin bin/.flags, pas d'une
# relecture du Makefile : les deux peuvent differer (make CFLAGS=...).
{
  echo "date        : $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "commit      : $(git describe --always --dirty --tags 2>/dev/null || echo inconnu)"
  echo "machine     : $(uname -srm)"
  echo "compilation : $(cat bin/.flags 2>/dev/null || echo inconnu)"
  echo "version cc  : $(${CC:-cc} --version 2>&1 | head -1)"
  echo "SEED=$SEED SCALE=$SCALE ASTAR_BYTES=$ASTAR_BYTES IDA_NODES=$IDA_NODES"
} > "$NEW/ENVIRONMENT.txt"
cat "$NEW/ENVIRONMENT.txt"
echo

# run <fichier> <binaire> <args...>
# Concatene en gardant un seul en-tete. Verifie que le CSV n'est pas vide :
# sans ce controle, une invocation ratee ferait recopier silencieusement le
# CSV de l'appel precedent.
run() {
  local dest="$1"; shift
  local tmp="$NEW/.tmp.csv"
  rm -f "$tmp"
  local rc=0
  "$@" --csv "$tmp" --quiet >/dev/null || rc=$?
  # 1 = --verify a trouve une solution non optimale : attendu pour dup=naive.
  [ "$rc" -le 1 ] || { echo "ECHEC (code $rc) : $*" >&2; exit "$rc"; }
  [ -s "$tmp" ] || { echo "CSV vide : $*" >&2; exit 1; }
  if [ -s "$NEW/$dest" ]; then tail -n +2 "$tmp" >> "$NEW/$dest"
  else cat "$tmp" > "$NEW/$dest"; fi
  rm -f "$tmp"
}

T=./bin/taquin
T15=./bin/taquin15

# --- 1. Les quatre heuristiques, memes instances ---------------------------
# Deux melanges, un pair et un impair : le graphe du taquin est biparti, un
# melange pair ne produit que des profondeurs paires. Echantillonner les deux
# parites evite un corpus tire de la moitie de l'espace d'etats.
for h in zero misplaced manhattan linear; do
  for sh in 200 201; do
    run astar-heuristics.csv $T --algo astar --impl fast --heuristic "$h" \
        --dup gcompare --seed "$SEED" --runs "$(n 500)" --shuffle "$sh"
  done
  echo "  heuristique $h : fait"
done

# --- 2. BFS contre A* ------------------------------------------------------
for algo in bfs astar; do
  for sh in 200 201; do
    run bfs-vs-astar.csv $T --algo "$algo" --impl fast --heuristic manhattan \
        --dup gcompare --seed "$SEED" --runs "$(n 500)" --shuffle "$sh"
  done
  echo "  algo $algo : fait"
done

# --- 3. Avant / apres les structures de donnees ----------------------------
# 200 instances seulement : l'implementation sur listes est en O(n^2).
for impl in list fast; do
  run impl-list-vs-fast.csv $T --algo astar --impl "$impl" \
      --heuristic manhattan --dup gcompare --seed "$SEED" \
      --runs "$(n 200)" --shuffle 200
  echo "  impl $impl : fait"
done

# --- 4. Politique de doublons, verifiee contre l'oracle --------------------
for dup in naive gcompare; do
  run dup-policy.csv $T --algo astar --impl fast --heuristic manhattan \
      --dup "$dup" --seed "$SEED" --runs "$(n 1000)" --shuffle 200 --verify
  echo "  dup $dup : fait"
done

# --- 5. Le mur memoire du 15-puzzle ----------------------------------------
# A* borne en MEMOIRE, IDA* borne en NOEUDS : chacun par la ressource qu'il
# epuise reellement. Un plafond unique en noeuds inverserait le resultat,
# puisque IDA* les cumule sur toutes ses iterations.
for sh in 20 40 60 80 100 140 200; do
  run puzzle15.csv $T15 --algo astar --impl fast --heuristic manhattan \
      --dup gcompare --seed "$SEED" --runs "$(n 10)" --shuffle "$sh" \
      --max-bytes "$ASTAR_BYTES"
  run puzzle15.csv $T15 --algo ida --heuristic manhattan \
      --seed "$SEED" --runs "$(n 10)" --shuffle "$sh" \
      --max-nodes "$IDA_NODES"
  echo "  15-puzzle melange $sh : fait"
done

# --- bascule atomique ------------------------------------------------------
rm -f "$NEW/.tmp.csv"
mkdir -p "$OUT"
for f in "$NEW"/*; do mv "$f" "$OUT/"; done
trap - EXIT
rmdir "$NEW" 2>/dev/null || true

echo
echo "CSV produits dans $OUT/ :"
wc -l "$OUT"/*.csv
