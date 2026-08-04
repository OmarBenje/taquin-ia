# Rétrospective

Ce que j'ai mesuré, ce qui a raté, ce que je ferais autrement.

---

## Ce que j'ai mesuré, et ce que ça a montré

Le point de départ était un solveur qui rendait une longueur de solution et deux
tailles de liste. On ne pouvait rien en prouver. La première chose faite a donc
été d'instrumenter — avant d'optimiser quoi que ce soit — et c'est ce qui a
rendu tout le reste possible.

Quatre résultats sont sortis de là :

**A\* développe 176× moins de nœuds que le BFS**, à solution identique. C'est le
résultat attendu, et l'oracle BFS est ce qui permet de l'affirmer : il explore
par profondeur croissante, donc sa première solution est optimale par
construction, sans rien supposer de l'heuristique.

**Les structures de données valent 161× de débit.** Remplacer `popBest` (O(n))
par un tas binaire et `onList` (O(n) avec un `memcmp` par nœud) par une table de
hachage fait passer de 125 713 à 20 209 800 nœuds générés par seconde. Le débit
de la version sur listes n'est d'ailleurs pas une constante : il s'effondre à
mesure que les listes grandissent. C'est la signature du O(n²).

**Jeter un doublon sans comparer son `g` coûte 11,6 % de solutions non
optimales.** C'était le comportement du code d'origine. Je m'attendais à une
subtilité théorique ; c'est un bug qui se déclenche une fois sur neuf.

**Le générateur d'instances n'est pas uniforme.** Le graphe du taquin est
biparti, donc un mélange de 200 coups ne peut jamais laisser la case vide sur un
bord, et à parité fixée la loi est proportionnelle au degré du sommet. Prédiction
et mesure coïncident à trois décimales. Ce résultat n'était pas dans le plan
initial : il vient d'une relecture qui a montré que ma prédiction de départ
(8,33 % / 12,50 % / 16,67 %) était fausse.

---

## Ce qui a raté

### Un tas presque trié, et une réponse presque optimale

`sift_down` comparait à `h->v[best]` au lieu du nœud en cours de descente. Après
la première descente, `h->v[best]` contient encore l'ancienne valeur remontée :
la comparaison portait sur un élément périmé.

Le résultat n'était pas un crash. Le tas restait *presque* trié, les `f`
sortaient dans le bon ordre la plupart du temps, et A\* rendait une solution
*presque* optimale — trop longue de deux coups, sur quelques instances
seulement.

Le test qui vérifiait « les `f` sortent croissants » passait. Ce qui l'a attrapé,
c'est la vérification de l'invariant du tas **après chaque insertion et chaque
extraction**.

**La leçon** : tester la structure, pas seulement le comportement observable. Un
comportement observable presque correct est le pire des cas — il passe les tests
et ment sur les résultats.

### Une macro du squelette qui déborde

`RANDMAX(x)` passe par un `float`. Pour `x = 4` et `rand()` proche de `RAND_MAX`,
le produit s'arrondit à `4 × 2³¹` et la macro rend exactement `4`, soit un accès
hors bornes dans `neighbors[4]`. Jamais observé en pratique, mais présent dans le
code depuis le début. Je ne l'ai pas trouvé en lisant le code : je l'ai trouvé en
me demandant pourquoi je remplaçais `rand()`.

### Une assertion de dominance trop forte, dans mes propres tests

J'ai écrit un test affirmant qu'une heuristique plus informée développe moins de
nœuds sur **chaque** instance. C'est faux. La dominance `h1 ≤ h2` ne garantit
rien sur les nœuds de coût `f = C*` : ils peuvent être développés ou non selon la
façon de départager les égalités, et une heuristique plus forte peut en toucher
davantage. La garantie ne porte que sur les nœuds de `f < C*`.

Le test comparait donc quelque chose de faux, et il échouait pour la bonne
raison. Il compare maintenant les totaux sur l'échantillon, où l'ordre est net.

### Le README a menti pendant neuf commits

En restructurant le dépôt, j'ai déplacé les binaires dans `bin/` sans mettre à
jour le README. La seule commande qu'il donnait, `./taquin_vs`, échouait. C'est
la première chose que tape un lecteur, et il en conclut que le projet ne compile
pas. Personne ne l'a vu parce que personne n'a relu le README en le suivant à la
lettre.

### Le Makefile ne recompilait rien

Pas de dépendances d'en-tête : `touch src/stats.h && make` répondait « Nothing to
be done ». Et `make CFLAGS="-O0"` non plus, parce que make compare les dates et
pas les options. J'aurais mesuré des binaires périmés en croyant mesurer mes
modifications, et `ENVIRONMENT.txt` aurait enregistré des flags que le binaire
n'avait jamais vus.

### Deux pièges évités de justesse

Une instruction placée entre `switch (…) {` et la première étiquette `case`
n'est **jamais** exécutée, et `-Wall -Wextra` ne dit rien. Le branchement d'IDA\*
y était : il serait tombé dans A\*, l'aurait exécuté en silence, et aurait écrit
`algo=ida` dans le CSV. Tous les tests de longueur seraient passés. Ce qui le
détecte désormais est un contrôle structurel : IDA\* ne ferme aucun état.

Et borner A\* et IDA\* par le même compteur de nœuds **inverse** la conclusion de
la phase 4 : avec un plafond commun, A\* résout 6/6 et IDA\* abandonne 3/6, parce
qu'IDA\* cumule ses nœuds sur toutes ses itérations. Le graphe censé montrer
qu'A\* meurt aurait montré l'inverse.

---

## Ce que je ferais autrement

**Instrumenter le premier jour, pas après coup.** Toutes les décisions des
phases 2 à 4 ont été prises sur des chiffres. Celles des phases 0 et 1 ont été
prises à l'intuition, et deux d'entre elles étaient fausses.

**Écrire les tests structurels avant les tests de résultat.** Les deux bugs les
plus coûteux (le tas, le branchement d'IDA\*) produisaient tous deux un résultat
plausible. Aucun test de sortie ne les attrape.

**Vérifier les prédictions avant de les écrire.** J'ai rédigé une spécification
affirmant que la loi du générateur serait 8,33 / 12,50 / 16,67 %. Trente
secondes de mesure auraient suffi à montrer que c'était faux — et le résultat
correct est bien plus intéressant.

**Des bases de motifs plutôt que le conflit linéaire.** Le conflit linéaire
divise les nœuds par 9 sur le 15-puzzle mais le temps par 1,3 seulement : il
coûte cher par nœud. Une PDB à 6 tuiles est le vrai saut, et c'est le genre de
travail qui distingue quelqu'un qui a lu Korf de quelqu'un qui a fait le TP.

**Un Manhattan incrémental.** Le delta d'un coup est en O(1) ; je recalcule la
distance entière sur chaque fils. Sur une instance à 10⁹ nœuds, c'est un facteur
2 à 3 laissé sur la table, et c'est le débit — pas la mémoire — qui limite IDA\*
sur le 15-puzzle.

**Un tas d'indices plutôt que de pointeurs**, pour la localité de cache. Non
mesuré, donc non affirmé.
