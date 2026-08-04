# Les six questions

Chaque réponse cite un chiffre mesuré quand il en existe un, et la commande qui
le produit. Les chiffres viennent des CSV de [`results/`](../results/).

---

## 1. Pourquoi Manhattan est-elle admissible ? Et consistante ? Laquelle autorise à jeter un nœud déjà fermé ?

**Admissible** : `h(n) ≤ h*(n)` pour tout `n` — elle ne surestime jamais le coût
restant. Chaque tuile doit parcourir au moins sa distance de Manhattan jusqu'à
sa case but, et un coup ne déplace qu'une seule tuile d'une seule case. La somme
est donc une borne inférieure du nombre de coups.

**Consistante** : `h(n) ≤ c(n, n') + h(n')` pour tout successeur `n'`. Ici
`c = 1`, et un coup change la distance de Manhattan totale d'exactement ±1
(la tuile déplacée se rapproche ou s'éloigne d'une case, les autres ne bougent
pas). Donc `h(n) − h(n') ∈ {−1, +1} ≤ 1`.

**C'est la consistance, pas l'admissibilité**, qui autorise à écarter
définitivement un nœud déjà fermé. Elle garantit qu'A\* extrait les nœuds par
`f` non décroissant, donc qu'un nœud extrait a déjà son `g` optimal. Avec une
heuristique seulement admissible, un nœud fermé peut encore être amélioré, et
il faut pouvoir le rouvrir.

*Mesuré* : `tests/test_heuristic.c` vérifie `h(n) ≤ 1 + h(fils)` sur 30 000
plateaux tirés au hasard et les quatre heuristiques — conflit linéaire compris.
399 698 vérifications, 0 échec.

---

## 2. Le conflit linéaire ajoute 2 par conflit. Prouve que ça reste admissible.

Deux tuiles sont en **conflit linéaire** si elles sont toutes deux dans leur
ligne but, et dans le mauvais ordre l'une par rapport à l'autre. Comme aucune ne
peut traverser l'autre, l'une des deux doit sortir de la ligne puis y revenir :
deux coups au minimum. Or ces deux coups s'annulent en distance de Manhattan,
donc Manhattan ne les compte pas. La somme `Manhattan + 2` reste donc une borne
inférieure : admissible.

**Le piège est le triplet.** Trois tuiles mutuellement en conflit forment trois
paires, mais faire sortir **deux** tuiles suffit à résoudre les trois conflits.
Ajouter `+6` surestimerait, et l'heuristique cesserait d'être admissible.

D'où le retrait glouton (Hansson, Mayer & Yung) : on retire répétitivement la
tuile qui participe au plus grand nombre de conflits, on compte `+2`, on efface
les conflits qu'elle portait. On compte ainsi le nombre **minimal** de tuiles à
faire sortir, ce qui reste une borne inférieure.

*Mesuré* : `tests/test_heuristic.c` vérifie les deux cas d'école — une paire en
conflit donne `+2`, un triplet mutuel donne `+4` et non `+6` — et que le
surcoût est toujours pair.

---

## 3. A\* jette un enfant déjà sur l'open list sans comparer `g`. Dans quel cas est-ce faux ?

**C'est faux en général**, et ça l'est ici aussi.

Un état présent sur l'open list n'a pas nécessairement son `g` optimal : seuls
les nœuds **extraits** l'ont, sous heuristique consistante. Jeter un fils sans
comparer fige donc le plus long des deux chemins.

La parité du graphe atténue le phénomène — deux chemins vers le même état ont
la même parité de longueur, donc ils diffèrent d'au moins 2, ce qui exige une
égalité exacte de `f` pour que le mauvais soit extrait en premier — mais elle ne
l'empêche pas.

*Mesuré*, sur 1 000 instances vérifiées contre l'oracle BFS :

| politique | non optimales | coups perdus |
|---|---:|---:|
| `--dup naive` (le code d'origine) | **116 / 1 000 (11,6 %)** | 232 |
| `--dup gcompare` | 0 / 1 000 | 0 |

```bash
./bin/taquin --dup naive --runs 1000 --shuffle 200 --seed 1 --verify
```

Le taux dépend fortement de la profondeur : sur les instances courtes des tests
(mélange 6 à 21) il tombe à 2 sur 120. C'est sur les instances difficiles que
la politique naïve coûte.

---

## 4. Complexité de `onList` avant et après ? Que hashes-tu, et comment gères-tu les collisions ?

**Avant** : `onList` parcourait la liste entière avec un `memcmp` par nœud, soit
O(n). A\* l'appelait deux fois par fils (closed puis open), donc huit fois par
nœud développé — la recherche était en O(n²) sur la taille des listes.

**Après** : table de hachage, O(1) amorti.

**Ce qui est haché** : les `MAX_BOARD` octets du plateau, et rien d'autre, en
FNV-1a 64 bits. Le plateau détermine entièrement l'état — la position de la case
vide s'en déduit — donc deux nœuds de même plateau sont le même état, quels que
soient leur `g` ou leur parent.

**Les collisions** : chaînage par seau (`node_t::hnext`), avec un `memcmp`
complet sur chaque candidat. Une égalité de hachage n'est jamais prise pour une
égalité d'état. Le nombre de seaux est une puissance de deux (le modulo est un
masque) et double dès que la charge dépasse 1,0.

*Mesuré*, colonne `hash_probes` des CSV : **1,52 nœud examiné par recherche** sur
41 081 états distincts répartis dans 65 536 seaux. La promesse O(1) est comptée,
pas supposée.

*Effet* : **166× de débit** à longueur de solution identique
(122 428 → 20 359 496 nœuds/s).

---

## 5. IDA\* réexplore des nœuds. Combien de fois, et pourquoi c'est quand même gagnant ?

Le facteur de ré-exploration est `generated / last_iter_generated` : le total
divisé par ce qu'a coûté la dernière itération.

*Mesuré* :

| instances | facteur |
|---|---:|
| 8-puzzle, mélange 6 à 21 (tests) | **5,58** |
| 15-puzzle, mélange 200 | **1,50** |

Il reste borné parce que le nombre de nœuds sous un seuil croît
géométriquement : la dernière itération domine la somme de toutes les
précédentes, et d'autant plus que l'instance est profonde. On paie donc un
facteur **constant** en temps.

Et on gagne quoi ? Le passage de O(b^d) à O(d) en mémoire. Sur le 15-puzzle à
mélange 200 :

| | résolues | mémoire pic médiane |
|---|---:|---:|
| A\* / Manhattan, borné à 512 Mo | **5 / 10** | 123 Mo |
| IDA\* / Manhattan | **10 / 10** | **768 octets** |

Sur ces instances, O(b^d) n'est pas un coût : c'est une impossibilité.

**Attention à la comparaison des bornes.** A\* meurt en mémoire, IDA\* meurt en
temps. Les borner tous les deux en nœuds générés inverse le résultat, parce
qu'IDA\* les cumule sur toutes ses itérations : avec un plafond commun de
2×10⁷ nœuds, A\* résout 6/6 et IDA\* abandonne 3/6. D'où `--max-bytes` pour l'un
et `--max-nodes` pour l'autre.

---

## 6. 200 mouvements aléatoires depuis l'état but : est-ce une distribution uniforme ?

**Non, et deux fois plutôt qu'une.** Ce n'est pas un raisonnement, c'est une
mesure : le 8-puzzle n'a que 9!/2 = 181 440 états solvables, donc on énumère
l'espace entier depuis le but (`--enumerate`) et on compare.

**D'abord, le graphe est biparti.** Chaque coup échange la case vide avec une
voisine, donc change la parité de (ligne + colonne) de la case vide. Après un
nombre **pair** de coups, la case vide ne peut physiquement pas se trouver sur
un bord. La marche est périodique : elle n'a pas une loi limite, elle en a deux.

**Ensuite, à parité fixée, la loi est proportionnelle au degré du sommet**, pas
uniforme. Le degré ne dépend que de la case vide : 2 aux coins, 3 sur les bords,
4 au centre.

*Mesuré*, 200 000 tirages :

| case vide | observé | H1 uniforme | H2 ∝ degré | H3 ∝ degré à parité fixée |
|---|---:|---:|---:|---:|
| coin (×4) | 16,61 à 16,74 % | 11,11 % | 8,33 % | **16,67 %** |
| bord (×4) | **0,00 %** | 11,11 % | 12,50 % | **0,00 %** |
| centre | 33,35 % | 11,11 % | 16,67 % | **33,33 %** |

| hypothèse | khi-deux | ddl | verdict |
|---|---:|---:|---|
| H1 — uniforme | 200 082 | 8 | **rejetée** |
| H2 — ∝ degré | 200 004 | 8 | **rejetée** |
| H3 — ∝ degré, à parité fixée | **2,0** | 4 | compatible (p = 0,73) |

```bash
./bin/taquin --dist 200000 --shuffle 200 --seed 1
```

La distribution des **profondeurs optimales**, même restreinte à la parité
atteignable, est elle aussi rejetée (p = 2×10⁻¹⁸) : les états profonds n'ont pas
les mêmes cases vides, donc le biais de degré les touche différemment.

**Conséquence pratique.** Le banc de ce dépôt tire donc chaque expérience à
mélange 200 **et** 201, faute de quoi tout le corpus viendrait de la moitié de
l'espace d'états. Un générateur réellement uniforme demanderait de tirer une
permutation au hasard et de rejeter les insolubles, ou de tirer un rang au
hasard dans `[0, 9!/2[` et de le dé-ranger — ce que `perm_unrank` sait faire.
