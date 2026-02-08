# Conception du projet de simulation ACO pour le fourragement

## Contexte et résumé
Ce projet consiste à simuler un modèle simple d’optimisation par colonies de fourmis (ACO) appliqué au fourragement sur une grille cartésienne 2D.
Chaque cellule porte une unité de temps de traversée comprise entre 0 et 1 et un type (libre, indésirable, fourmilière, nourriture).
Une population de *m* fourmis se déplace selon un voisinage à quatre directions et interagit indirectement via deux phéromones *V1* et *V2* stockées par cellule.
Les phéromones sont mises à jour localement à partir du maximum et de la moyenne des voisins, avec des cas particuliers pour la nourriture et le nid.
Un mécanisme d’évaporation applique un coefficient *β* aux phéromones à chaque pas de temps.
L’environnement (carte des unités de temps) est généré stochastiquement via un algorithme de « plasma » sur sous‑grilles recouvrantes, avec une déviation *d* et une normalisation dans [0,1].
Le projet demande une instrumentation des temps d’exécution par itération et par parties du code, puis une comparaison entre versions.
Une version vectorisée, une parallélisation en mémoire partagée (OpenMP) et deux stratégies de parallélisation distribuée doivent être étudiées via des tableaux d’accélération.

### Glossaire
- **ACO (Ant Colony Optimization)** : famille d’algorithmes d’intelligence en essaim ; ici, modèle de fourragement visant le chemin le plus court entre fourmilière et nourriture.
- **Grille 2D** : terrain représenté par une grille cartésienne ; chaque cellule *s* porte des données (type, unité de temps, phéromones).
- **Unité de temps** : valeur par cellule traduisant la difficulté de traversée, comprise entre 0 et 1.
- **Cellule indésirable** : cellule non traversable par les fourmis.
- **Fourmilière (nid)** : cellule représentant le point de retour des fourmis ; met *V2(s)=1* et remet l’état à « non chargée ».
- **Nourriture** : cellule source de nourriture (potentiellement non unique) ; met *V1(s)=1* et met l’état à « chargée ».
- **Phéromones *V1(s)* / *V2(s)*** : valeurs réelles par cellule ; *V1* guide l’exploration, *V2* guide le retour au nid.
- **Voisinage *N(s)*** : ensemble des quatre cellules voisines de *s* (haut, bas, gauche, droite).
- **Paramètre *α*** : paramètre de bruit dans la mise à jour des phéromones, avec 0 ≤ *α* ≤ 1.
- **Paramètre *ε*** : taux d’exploration dans le déplacement, avec 0 ≤ *ε* ≤ 1.
- **Paramètre *β*** : coefficient d’évaporation des phéromones, typiquement proche de 1.
- **Plasma** : procédure stochastique de génération d’une carte lisse (gradient borné) par subdivision récursive de sous‑grilles.
- **Sous‑grille / recouvrement** : la grille est composée de *n × n* sous‑grilles de taille *n_s = 2^k + 1* par direction, recouvrant leurs voisines via une rangée de cellules.
- **Déviation *d*** : borne utilisée pour limiter les variations lors de la génération du plasma.
- **OpenMP** : API de parallélisation en mémoire partagée pour paralléliser des boucles (threads).
- **Parallélisation distribuée (processus)** : exécution multi‑processus ; deux stratégies sont décrites (environnement complet par processus, ou découpage spatial).
- **Accélération** : mesure de gain de performance en fonction du nombre de threads/processus (définition précisée dans « Hypothèses / Décisions de conception »).

## Objectif général
Concevoir une simulation du fourragement par ACO sur une grille 2D, sur un environnement généré par plasma, et produire une étude de performance comparant une version de référence, une version vectorisée, une parallélisation OpenMP et des parallélisations distribuées, à l’aide de mesures de temps et de tableaux d’accélération.

## Objectifs spécifiques
1. Représenter l’environnement comme une grille 2D avec unités de temps et types de cellules (libres, indésirables, fourmilière, nourriture).
2. Simuler une population de *m* fourmis avec positions, états (chargée / non chargée) et compteur global de nourriture.
3. Mettre à jour les phéromones *V1(s)* et *V2(s)* via les statistiques des voisins (maximum et moyenne) et le paramètre *α*, avec cas particuliers (nourriture / nid).
4. Implémenter le déplacement par exploration (*ε*) ou exploitation (*1−ε*), en interdisant les cellules indésirables.
5. Implémenter l’évaporation des phéromones selon le coefficient *β*.
6. Générer l’environnement par plasma à partir des paramètres *n*, *n_s = 2^k + 1*, *d* et d’une graine, avec recouvrement des sous‑grilles et normalisation dans [0,1].
7. Mesurer et consigner le temps passé par itération et par parties du code.
8. Produire une version vectorisée où les fourmis sont représentées par des tableaux (coordonnées, états, graines, etc.) et identifiées par leur indice.
9. Paralléliser en mémoire partagée avec OpenMP, mesurer les temps selon le nombre de threads et dresser un tableau d’accélération.
10. Paralléliser en distribué selon deux stratégies, mesurer les temps selon le nombre de processus (un cœur par processus) et calculer l’accélération correspondante.

## Périmètre

### Inclus
- Simulation sur grille 2D avec deux phéromones *V1*/*V2* et règles de mise à jour/déplacement décrites.
- Gestion des états (chargée / non chargée) et du compteur global d’unités de nourriture.
- Évaporation des phéromones via *β*.
- Génération d’environnement par plasma (sous‑grilles recouvrantes, déviation *d*, normalisation).
- Mesure du temps passé par itération et dans différentes parties du code.
- Version vectorisée (tableaux de données des fourmis).
- Parallélisation mémoire partagée via OpenMP, avec tableau d’accélération selon le nombre de threads.
- Parallélisation distribuée selon deux méthodes décrites, avec mesures et accélération selon le nombre de processus (et bonus d’implémentation pour la seconde méthode).

### Exclus
- Interface utilisateur avancée, visualisation élaborée ou interaction temps réel non demandées.
- Persistance (sauvegarde/chargement), base de données, service réseau.
- Optimisations matérielles non mentionnées (GPU, accélérateurs dédiés).
- Variantes d’algorithmes d’optimisation autres que le modèle ACO décrit.

### Contraintes
- **Paramètres du modèle ACO** : nombre de fourmis *m* ; *α* (0 ≤ *α* ≤ 1) ; *ε* (0 ≤ *ε* ≤ 1) ; *β* (évaporation, typiquement proche de 1).
- **Terrain** : unité de temps par cellule dans [0,1] ; quatre types de cellules (fourmilière, nourriture potentiellement multiple, indésirables non traversables, libres).
- **Voisinage** : quatre voisines (haut, bas, gauche, droite).
- **Phéromones** : deux champs réels *V1(s)* et *V2(s)* par cellule, initialisés à 0.
- **Initialisation** : positions initiales des *m* fourmis soit toutes à la fourmilière, soit réparties uniformément sur la grille ; toutes non chargées ; compteur nourriture initial à 0.
- **Plasma** : grille composée de *n × n* sous‑grilles ; taille par sous‑grille *n_s = 2^k + 1* ; recouvrement par une rangée de cellules ; déviation *d* ; génération dépendante d’une graine et de la coordonnée des sommets ; normalisation finale dans [0,1].
- **Évaluation des performances** : mesures répétées sur versions (référence, vectorisée, OpenMP, distribuée) et production de tableaux d’accélération en fonction du nombre de threads et de processus.

### Hypothèses / Décisions de conception
- **Bords de la grille (voisinage)** : hors de la grille, les voisines sont considérées comme non traversables et leurs phéromones sont prises comme 0 pour le calcul du maximum et de la moyenne.
- **Moyenne des voisins** : le facteur 1/4 est conservé tel que décrit ; les voisines hors‑grille comptent comme 0 (cf. décision ci‑dessus).
- **Encodage des cellules indésirables** : une cellule indésirable est représentée par une valeur d’unité de temps égale à −1 (pour correspondre à la règle « valeur −1 » lors du déplacement).
- **Égalités lors du choix “meilleure voisine”** : en cas d’égalité, la fourmi choisit aléatoirement parmi les voisines maximales, via son générateur pseudo‑aléatoire.
- **Aucune voisine traversable** : la fourmi reste sur place et ne se déplace plus durant ce pas de temps.
- **Ordonnancement d’une itération** : (1) pour chaque fourmi, mise à jour des phéromones sur la cellule courante puis déplacement(s) selon le budget de mouvement ; (2) évaporation globale des phéromones sur la grille.
- **Budget de mouvement** : le coût d’un déplacement est pris égal à l’unité de temps de la cellule de destination. Pour éviter un nombre non borné de déplacements si le coût vaut 0, un plafond de déplacements par fourmi et par pas de temps est fixé.
- **Définition de l’accélération** : l’accélération pour *p* threads/processus est définie par *S(p) = T(1) / T(p)*, où *T(p)* est un temps mesuré sur un scénario identique (mêmes paramètres et mêmes graines).
- **Parallélisation distribuée — technologie** : le document parle de « processus » et d’échanges ; aucune API de communication n’est imposée à ce stade.
- **Parallélisation distribuée (méthode 2)** : la gestion des bords est basée sur des échanges de bandes (halo) des valeurs de phéromones et sur la migration des fourmis franchissant une frontière de sous‑carte.

## Exigences fonctionnelles
- **FR-01 — Représentation de la grille** : le système doit représenter un terrain sous forme de grille 2D ; chaque cellule doit porter (i) un type parmi {libre, indésirable, fourmilière, nourriture} et (ii) une unité de temps de traversée.
- **FR-02 — Cellules spéciales** : le système doit permettre de définir au moins une cellule fourmilière et au moins une cellule nourriture ; il doit permettre la présence de plusieurs cellules nourriture.
- **FR-03 — Stockage des phéromones** : chaque cellule *s* doit stocker deux valeurs réelles *V1(s)* et *V2(s)*, initialisées à 0 au démarrage de la simulation.
- **FR-04 — Initialisation des fourmis** : le système doit initialiser *m* fourmis avec (a) une position et (b) un état « non chargée » ; l’initialisation des positions doit être configurable entre « toutes à la fourmilière » et « répartition uniforme sur la grille ».
- **FR-05 — Compteur de nourriture** : le système doit maintenir un compteur global d’unités de nourriture, initialisé à 0.
- **FR-06 — Transition d’état (nourriture)** : lorsqu’une fourmi arrive sur une cellule nourriture, son état doit devenir « chargée ».
- **FR-07 — Transition d’état (fourmilière)** : lorsqu’une fourmi arrive sur une cellule fourmilière, son état doit devenir « non chargée ».
- **FR-08 — Incrément du compteur** : lorsqu’une fourmi arrive à la fourmilière avec l’état « chargée », le compteur global d’unités de nourriture doit être incrémenté d’une unité.
- **FR-09 — Calcul des statistiques locales** : pour une cellule *s*, le système doit pouvoir calculer *max_i(N(s))* et *avg_i(N(s))* à partir des valeurs *V_i* des quatre voisines *N(s)*, pour *i ∈ {1,2}*.
- **FR-10 — Mise à jour de *V1*** : pour une fourmi sur *s*, la mise à jour de *V1(s)* doit respecter : *V1(s)=1* si *s* est une cellule nourriture ; sinon *V1(s)=α·max_1(N(s)) + (1−α)·avg_1(N(s))*.
- **FR-11 — Mise à jour de *V2*** : pour une fourmi sur *s*, la mise à jour de *V2(s)* doit respecter : *V2(s)=1* si *s* est la fourmilière ; sinon *V2(s)=α·max_2(N(s)) + (1−α)·avg_2(N(s))*.
- **FR-12 — Déplacement (exploration/exploitation)** : à chaque décision de déplacement, une fourmi doit choisir une cellule voisine traversable soit (a) au hasard avec probabilité *ε*, soit (b) par maximisation de *V1* (fourmi non chargée) ou de *V2* (fourmi chargée) avec probabilité *1−ε*.
- **FR-13 — Interdiction des cellules indésirables** : une fourmi ne doit jamais se déplacer sur une cellule indésirable (cellule non traversable).
- **FR-14 — Budget de mouvement** : à chaque pas de temps, une fourmi doit disposer d’un budget de mouvement de 1 et enchaîner des déplacements tant que ce budget n’est pas épuisé, en consommant le budget selon l’unité de temps des cellules traversées (voir « Hypothèses / Décisions de conception »).
- **FR-15 — Évaporation** : à chaque pas de temps, le système doit appliquer l’évaporation *V_i(s) ← β·V_i(s)* pour tout *s* de la grille et pour *i ∈ {1,2}*.
- **FR-16 — Génération plasma (structure)** : le système doit générer une carte via un plasma composé de *n × n* sous‑grilles de taille *n_s = 2^k + 1*, avec recouvrement par une rangée de cellules entre sous‑grilles adjacentes.
- **FR-17 — Génération plasma (déviation et bords)** : le système doit générer les coins puis subdiviser récursivement chaque sous‑grille en respectant des déviations bornées par *d × n_s* puis *d × (n_s/2)*, en assurant la cohérence des bords communs entre sous‑grilles.
- **FR-18 — Normalisation plasma** : après génération, la carte des unités de temps doit être normalisée afin que toutes les valeurs soient comprises entre 0 et 1.
- **FR-19 — Mesure des temps** : le système doit mesurer et consigner le temps passé par itération et par parties du code, afin de comparer la version de référence, la version vectorisée, la version OpenMP et les versions distribuées.
- **FR-20 — Vectorisation (données des fourmis)** : la version vectorisée doit représenter les fourmis via des tableaux (coordonnées, états, graines, etc.) et repérer une fourmi par son indice dans ces tableaux.
- **FR-21 — Parallélisation OpenMP** : le système doit proposer une version parallélisée en mémoire partagée via OpenMP et produire un tableau d’accélération en fonction du nombre de threads (et du nombre de cœurs de la machine utilisée).
- **FR-22 — Distribué, méthode 1** : le système doit proposer une version distribuée où chaque processus possède l’environnement complet, ne gère qu’une partie des fourmis et calcule l’évaporation sur une partie de la carte ; la fusion des phéromones entre processus doit retenir la valeur maximale par cellule.
- **FR-23 — Distribué, méthode 2 (stratégie)** : le système doit définir une stratégie de découpage spatial où chaque processus gère une sous‑carte et les fourmis présentes, avec gestion explicite des bords (voir « Hypothèses / Décisions de conception ») ; en cas d’implémentation, il doit être possible de mesurer l’accélération en fonction du nombre de processus.

## Exigences non fonctionnelles
- **NFR-01 — Performance mesurée** : les résultats doivent inclure des mesures de temps comparables entre versions (référence, vectorisée, OpenMP, distribuée) obtenues sur des scénarios identiques (mêmes paramètres et mêmes graines).
- **NFR-02 — Scalabilité (threads)** : une campagne de mesures doit produire une accélération *S(p)* pour plusieurs valeurs de *p* (nombre de threads) et fournir le tableau correspondant.
- **NFR-03 — Scalabilité (processus)** : une campagne de mesures doit produire une accélération *S(p)* pour plusieurs valeurs de *p* (nombre de processus, un cœur par processus) et fournir le tableau correspondant pour les méthodes distribuées étudiées.
- **NFR-04 — Reproductibilité contrôlée** : le système doit permettre de fixer les graines utilisées pour la génération du plasma et pour les tirages aléatoires des fourmis afin de reproduire un scénario de référence.
- **NFR-05 — Correction des phéromones** : la mise à jour locale (*max* + *moyenne*) et l’évaporation (*β*) doivent être appliquées conformément aux formules ; en mode distribué, la cohérence globale doit être assurée (fusion par maximum pour la méthode 1 ; cohérence aux bords pour la méthode 2).
- **NFR-06 — Robustesse aux paramètres** : le système doit fonctionner pour différentes tailles de grilles, différentes valeurs de *m*, et des valeurs extrêmes de *ε* (0 et 1), sans produire d’états invalides (fourmis hors‑grille, sur cellules indésirables, etc.).
- **NFR-07 — Maintenabilité** : l’architecture doit séparer clairement la génération d’environnement, la simulation (mise à jour/déplacement/évaporation), l’instrumentation des temps et les variantes de parallélisation afin de faciliter les comparaisons.
- **NFR-08 — Portabilité** : la solution doit rester exécutable sur une plateforme standard supportant OpenMP pour la mémoire partagée et une exécution multi‑processus pour la parallélisation distribuée.

## Traçabilité — Exigences entièrement satisfaites
| ID | Exigence (résumé) | Preuves (fichiers + symboles) |
|---|---|---|
| FR-03 | Stocker deux phéromones par cellule et initialiser la carte | src/pheronome.hpp::class pheronome (m_map_of_pheronome std::array<double,2>; constructeur lignes 30-44) |
| FR-05 | Maintenir un compteur global d’unités de nourriture | src/ant_simu.cpp:64-80 (variable `food_quantity`, boucle principale) ; src/ant.cpp::advance (incrément `cpteur_food`) |
| FR-06 | Passer l’état d’une fourmi à « chargée » en arrivant sur une cellule nourriture | src/ant.cpp::advance (lignes 54-56, `set_loaded()` lorsque position == pos_food) |
| FR-07 | Passer l’état d’une fourmi à « non chargée » en arrivant sur la fourmilière | src/ant.cpp::advance (lignes 48-53, `unset_loaded()` lorsque position == pos_nest) |
| FR-08 | Incrémenter le compteur lorsqu’une fourmi chargée atteint le nid | src/ant.cpp::advance (lignes 48-51, `cpteur_food += 1`) |
| FR-09 | Calculer max et moyenne des quatre voisines pour chaque phéromone | src/pheronome.hpp::mark_pheronome (lignes 80-99, calcul max/avg sur voisins) |
| FR-10 | Mettre à jour V1 avec cas nourriture (=1) sinon mélange max/moyenne avec α | src/pheronome.hpp::mark_pheronome (lignes 93-99) ; src/pheronome.hpp::update (lignes 101-106, remise V1=1 sur pos_food) |
| FR-11 | Mettre à jour V2 avec cas nid (=1) sinon mélange max/moyenne avec α | src/pheronome.hpp::mark_pheronome (lignes 96-99) ; src/pheronome.hpp::update (lignes 101-106, remise V2=1 sur pos_nest) |
| FR-12 | Déplacer une fourmi par exploration ε ou exploitation 1−ε sur la meilleure phéromone | src/ant.cpp::advance (lignes 15-44, choix aléatoire vs max V1/V2 selon état) |
| FR-13 | Interdire les cellules indésirables (valeur −1) lors du déplacement | src/ant.cpp::advance (lignes 25-34, boucle jusqu’à `phen[new_pos_ant][ind_pher] != -1`); src/pheronome.hpp::cl_update (lignes 119-126, bords marqués à -1) |
| FR-15 | Appliquer l’évaporation des phéromones avec β à chaque pas | src/pheronome.hpp::do_evaporation (lignes 65-71) ; src/ant_simu.cpp:15-19 (appel `do_evaporation()` puis `update()`) |
| FR-18 | Normaliser la carte du plasma dans [0,1] | src/ant_simu.cpp:37-50 (calcul min/max puis remise à l’échelle 0–1) |
