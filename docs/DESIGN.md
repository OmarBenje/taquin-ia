# Décisions de conception

Ce que j'ai choisi, et pourquoi. Écrit après coup, à partir de ce que les
mesures ont montré.

---

## Les règles du jeu vivent dans `puzzle.c`, pas dans `board.c`

Le squelette de l'école mélangeait les règles du taquin et la structure de nœud
`Item` de sa liste chaînée. Les extraire dans un module qui ne connaît qu'un
tableau d'octets a eu trois effets qui n'étaient pas visibles au départ :

- le solveur rapide et le solveur historique partagent exactement les mêmes
  règles, donc leur désaccord ne peut venir que de l'algorithme ;
- `WH_BOARD` devient surchargeable à la compilation, et le 15-puzzle n'a
  demandé aucune ligne de code, seulement une cible de Makefile ;
- `puzzle_apply` supporte `dst == src`, ce qui permet à IDA\* de muter un
  unique plateau et de le restaurer, au lieu d'en copier un par nœud.

## Un générateur pseudo-aléatoire maison plutôt que `rand()`

Deux raisons, aucune esthétique.

`rand()` n'est pas spécifié : la suite produite par `srand(42)` diffère entre la
libc de macOS et la glibc. Un `--seed 42` ne rejouerait pas la même instance
d'une machine à l'autre, ce qui vide `--seed` de son sens.

Et la macro `RANDMAX(x)` du squelette passe par un `float` :
`(int)((float)(x) * rand() / (RAND_MAX + 1.0))`. Pour `x = 4` et `rand()` proche
de `RAND_MAX`, le produit `4 × 2147483647 = 8589934588` s'arrondit au `float` le
plus proche, `8589934592 = 4 × 2³¹`, et la macro rend exactement `4` — soit un
accès hors bornes dans le `neighbors[4]` d'`initGame`. Jamais observé, mais
présent.

## `g` et `h` en entiers, et un départage explicite à `f` égal

Comparer des `f` flottants pour l'égalité est fragile, or « `f` égaux » doit
être une notion sûre pour pouvoir départager. En entiers, elle l'est.

À `f` égal, le tas préfère le plus grand `g`. Manhattan ne varie que de ±1 par
coup, donc les plateaux de `f` identiques sont énormes ; départager vers la
profondeur atteint le but plus tôt sans rien coûter à l'optimalité, puisqu'A\*
ne s'arrête qu'en extrayant le but. Effet mesuré : 122 291 → 66 057 nœuds
générés sur les mêmes 30 instances.

## IDA\* n'a pas de table d'états visités

C'est le choix central de la phase 4, et il est contre-intuitif : ajouter une
table réduirait le nombre de nœuds explorés. Mais c'est elle qui ramènerait la
mémoire à O(b^d), et la mémoire est tout ce qu'IDA\* a à offrir. La seule
élimination est donc l'anti-retour immédiat.

Le coût est réel : facteur de ré-exploration mesuré à 1,50 sur le 15-puzzle et
5,58 sur des instances courtes. Il reste borné parce que le nombre de nœuds sous
un seuil croît géométriquement, donc la dernière itération domine.

**Corollaire à connaître** : IDA\* est incomplet « vers le négatif ». Sur un
plateau insoluble il relèverait son seuil indéfiniment sans jamais conclure.
`solve_ida` teste donc la parité des inversions avant de chercher.

## Deux bornes, pas une

A\* meurt en **mémoire**, IDA\* meurt en **temps**. Les borner tous les deux en
nœuds générés inverse le résultat : IDA\* les cumule sur toutes ses itérations,
donc un plafond commun de 2×10⁷ nœuds fait résoudre 6/6 à A\* et abandonner 3/6
à IDA\*. D'où `--max-bytes` pour l'un, `--max-nodes` pour l'autre.

C'est le genre de détail qui décide du sens d'un graphe.

## La mémoire d'IDA\* n'est pas un delta d'adresses de pile

La tentation était de mesurer la pile en comparant l'adresse d'un local à celle
prise à la racine. Mesuré, ce nombre varie de 13 % entre `-O0` et `-O2` : c'est
une propriété du compilateur, pas de l'algorithme, et l'inlining récursif peut
le rendre non proportionnel à la profondeur.

Le dépôt publie donc `open_max`, le nombre de nœuds retenus simultanément —
la seule grandeur qui compare la même chose des deux côtés.

## Le rang de Lehmer pour énumérer le 8-puzzle

`perm_rank` transforme un plateau en un entier de `[0, 9![`, ce qui permet
d'indexer un tableau de 362 880 octets et de faire un BFS complet depuis le but
en 0,15 s. C'est ce qui transforme la question « le générateur est-il uniforme »
d'une intuition en une mesure.

`perm_unrank` existe surtout pour le test : `rank(unrank(r)) == r` sur les
362 880 rangs **prouve** la bijection, là où tirer des plateaux au hasard ne
fait que l'échantillonner. La sûreté mémoire de l'énumération en dépend, sa
file étant dimensionnée sur le théorème 9!/2.

## matplotlib épinglé plutôt qu'un générateur SVG maison

matplotlib n'était pas installé sur la machine de développement, et j'ai
d'abord conclu qu'il fallait écrire les figures à la main. C'était un
non-sequitur : un `requirements.txt` épinglé est exactement aussi reproductible,
et une demi-journée passée à éviter une dépendance se lit mal.

## Ce qui reste hors périmètre

- **Bases de motifs (pattern databases).** C'est la suite logique du conflit
  linéaire et le vrai différenciateur du domaine : une PDB à 6 tuiles rendrait
  résolubles en secondes les instances qu'IDA\* met une minute à traiter.
- **Manhattan incrémental.** `heuristic_eval` recalcule la distance entière sur
  chaque fils, alors que le delta d'un coup est en O(1). Sur une instance à
  10⁹ nœuds, c'est un facteur 2 à 3 laissé sur la table.
- **Le 24-puzzle**, qui demande des bases de motifs pour être abordable.
- **La parallélisation.**
- **La réécriture de l'historique Git** pour purger les PDF des anciens
  commits : ils sont sortis du suivi, mais les purger de l'historique est une
  opération destructrice qui demande un accord explicite et un `push --force`.
