# Itération 1 — Simulation séquentielle stabilisée (10–14/02)

## Objectifs & livrables
- Simulation ACO séquentielle stable (pas de crash, pas de boucle infinie, pas de sortie de grille).
- Corrections de robustesse sur le déplacement et la mise à jour des phéromones.
- Suite de tests unitaires (phéromones/évaporation) + au moins 1 test d'intégration (états + compteur nourriture).
- Validation de la normalisation du paysage fractal dans [0,1].
- Documentation d'usage mise à jour (paramètres m, alpha, beta, epsilon + graines).
- Intégration sur la branche principale avec vérifications (build + exécution minimale).

## Exigences ciblées (FR / NFR)
- FR-01, FR-02, FR-03, FR-04, FR-05, FR-06, FR-07, FR-08, FR-09, FR-10, FR-11, FR-12, FR-13, FR-14, FR-15, FR-18
- NFR-04, NFR-05, NFR-06, NFR-07

## Plan de travail — Javier
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| J1-01 | Support CI locale: cibles build/test/smoke | P0 | 4 | — | `make test`/`make smoke` documentés et exécutables |
| J1-02 | Revue technique & alignement spec (quotidien) | P0 | 6 | D1-01..D1-06 (au fil de l'eau) | Commentaires de revue + validation des critères spec |
| J1-03 | Intégration (merge) + vérification bout-en-bout | P0 | 3 | D1-01..D1-08 | Merge + log build/tests + smoke run |
| J1-04 | Validation finale It1 (DoD) + go/no-go | P0 | 2 | J1-03 | Checklist DoD complétée et signée |

[Tâche J1-01] — Support CI locale: cibles build/test/smoke

Priorité : P0

Estimation : 4 h

Exigences liées : NFR-07 (et support indirect des FR-xx via tests)

Description :
Mettre en place une exécution locale reproductible des checks Itération 1 (build + tests + smoke run) pour réduire les frictions d'intégration et accélérer la validation.

Étapes :
- [ ] Ajouter une cible `test` qui compile et exécute un binaire de tests (sans ouvrir de fenêtre SDL).
- [ ] Ajouter une cible `smoke` qui lance une exécution courte et vérifie qu'elle termine (durée/itérations bornées).
- [ ] Vérifier `debug`/`release` (ou équivalent) et documenter les commandes.
- [ ] Documenter les commandes dans `README.md` (section "Tests / Smoke").

Fichiers / modules ciblés :
- `src/Makefile`
- `README.md`
- (Nouveaux fichiers de tests) `tests/` ou équivalent (à compiler via Makefile)

Dépendances :
- Démarrage Itération 0 terminé (toolchain OK).

Critères d'acceptation :
- `make test` compile et exécute les tests avec un code retour 0.
- `make smoke` termine en moins de 60s sur la machine de dev et retourne 0.
- Les commandes sont documentées et reproductibles.

Livrable / Preuve :
- Cibles Makefile + extrait `README.md` + sortie console (log) des commandes.

[Tâche J1-02] — Revue technique & alignement spec (quotidien)

Priorité : P0

Estimation : 6 h

Exigences liées : FR-01 à FR-15, FR-18 ; NFR-04, NFR-05, NFR-06, NFR-07

Description :
Assurer que les corrections et tests de Daniel respectent les formules et contraintes de `docs/conception.md` et les objectifs Itération 1, et prévenir les régressions (bords, seeds, boucle de mouvement).

Étapes :
- [ ] Relire chaque PR/commit majeur (au moins 1 revue/jour).
- [ ] Vérifier les points "pièges" : indices négatifs/unsigned, accès voisins, coût=0, cellules indésirables, remise à 1 sur nid/nourriture, seeds.
- [ ] Exiger au moins 1 test par correctif critique (ou justification).
- [ ] Valider la traçabilité FR/NFR dans les descriptions de PR.

Fichiers / modules ciblés :
- `src/ant.hpp`, `src/ant.cpp`
- `src/pheronome.hpp`
- `src/fractal_land.hpp`, `src/fractal_land.cpp`
- `src/ant_simu.cpp`
- `tests/*` (nouveau)

Dépendances :
- Travaux Daniel (D1-01..D1-06) au fil de l'eau.

Critères d'acceptation :
- Les modifications sont conformes aux exigences ciblées et n'ajoutent pas de fonctionnalités hors scope It1.
- Les cas bords (i=0/j=0, i=dim-1/j=dim-1) sont couverts par tests ou par preuve claire.
- Aucun warning/UB évident non traité sur les zones modifiées.

Livrable / Preuve :
- Commentaires de revue (PR) + checklist de revue signée (dans la description de merge).

[Tâche J1-03] — Intégration (merge) + vérification bout-en-bout

Priorité : P0

Estimation : 3 h

Exigences liées : FR-01 à FR-15, FR-18 ; NFR-04, NFR-05, NFR-06, NFR-07

Description :
Intégrer les changements It1 sur la branche principale et vérifier que le projet reste compilable/exécutable avec tests verts.

Étapes :
- [ ] Geler les changements It1 (feature freeze) le 14/02 midi.
- [ ] Merger la/les PR(s) It1 sur la branche principale.
- [ ] Lancer `make clean && make all` (debug et release si disponibles).
- [ ] Lancer `make test` et archiver la sortie.
- [ ] Lancer `make smoke` et archiver la sortie.

Fichiers / modules ciblés :
- Branche principale (intégration)
- `src/Makefile`, `tests/*`, modules modifiés pendant It1

Dépendances :
- D1-01..D1-08 terminées.
- J1-01 (cibles Makefile) en place.

Critères d'acceptation :
- Build OK + tests OK + smoke OK sur la machine de dev.
- Aucun conflit non résolu, pas de régression fonctionnelle visible.

Livrable / Preuve :
- Commit(s) de merge + logs des commandes (copiés dans la description du merge).

[Tâche J1-04] — Validation finale It1 (DoD) + go/no-go

Priorité : P0

Estimation : 2 h

Exigences liées : Critères d'acceptation Itération 1 (iterations.md)

Description :
Clore l'itération en validant les critères d'acceptation et en listant explicitement les éventuelles dettes reportées à It2.

Étapes :
- [ ] Vérifier la checklist "Definition of Done (Itération 1)" ci-dessous.
- [ ] Noter les écarts (si un FR reste partiel) et créer une action de suivi pour It2.
- [ ] Confirmer que la doc d'usage est à jour et cohérente avec le code.

Fichiers / modules ciblés :
- `README.md`
- `docs/conception.md` (lecture/validation uniquement)
- `tests/*` et sortie `make test`

Dépendances :
- J1-03 terminé.

Critères d'acceptation :
- Tous les items DoD sont cochés, ou bien un écart est explicitement documenté avec un plan It2.

Livrable / Preuve :
- DoD rempli + note de clôture (dans la description du merge ou un commentaire de release).

## Plan de travail — Daniel
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| D1-01 | Corriger seeds/RNG des fourmis (reproductibilité) | P0 | 2 | — | Déplacement aléatoire déterministe à graine fixée |
| D1-02 | Sécuriser accès voisins phéromones (bords) | P0 | 5 | D1-01 | Pas d'UB sur bords + test edge-case |
| D1-03 | Représentation types (nid/nourriture/indésirable/libre) + multi-nourriture | P0 | 7 | D1-02 | Support 1..N sources nourriture + cellules indésirables internes |
| D1-04 | Robustesse budget de mouvement (coût=0, bornes) | P0 | 3 | D1-02 | Pas de boucle infinie, mouvement borné |
| D1-05 | Mettre en place harness tests + unit tests phéromones/évaporation | P0 | 6 | D1-02 | `tests/*` + exécution via `make test` |
| D1-06 | Tests d'intégration: états fourmi + compteur nourriture | P1 | 4 | D1-03, D1-05 | Test intégration vert |
| D1-07 | Test normalisation paysage dans [0,1] | P1 | 2 | D1-05 | Test vert + preuve plage [0,1] |
| D1-08 | Doc d'usage (paramètres + graines) + validation finale Daniel | P1 | 3 | D1-05..D1-07 | `README.md` mis à jour + logs tests/smoke |

[Tâche D1-01] — Corriger seeds/RNG des fourmis (reproductibilité)

Priorité : P0

Estimation : 2 h

Exigences liées : FR-04, FR-12 ; NFR-04

Description :
Garantir que chaque fourmi dispose d'une graine initialisée et que les tirages aléatoires (exploration) sont reproductibles à paramètres et graines identiques. Cela conditionne la fiabilité des tests et des validations.

Étapes :
- [ ] Initialiser `ant::m_seed` dans le constructeur (`src/ant.hpp`) avec le paramètre `seed`.
- [ ] Vérifier que la création des fourmis dans `src/ant_simu.cpp` passe des graines distinctes et stables (par fourmi).
- [ ] Ajouter un test simple de reproductibilité (même graine -> même séquence de `rand_int32`/`rand_double` ou même premier déplacement sous scénario contrôlé).

Fichiers / modules ciblés :
- `src/ant.hpp`
- `src/ant.cpp`
- `src/ant_simu.cpp`
- `src/rand_generator.hpp`
- `tests/test_rng.cpp` (nouveau, si retenu)

Dépendances :
- Aucune (peut commencer immédiatement).

Critères d'acceptation :
- Aucune lecture de seed non initialisée (pas d'UB).
- Un test reproductible passe (deux runs consécutifs donnent le même résultat).

Livrable / Preuve :
- Diff de code + test vert (`make test`).

[Tâche D1-02] — Sécuriser accès voisins phéromones (bords)

Priorité : P0

Estimation : 5 h

Exigences liées : FR-09, FR-10, FR-11, FR-12, FR-13 ; NFR-05, NFR-06

Description :
Éliminer les conversions implicites int->size_t qui cassent les accès aux cellules fantômes (bords) et peuvent produire des accès mémoire hors limites lors du calcul des voisins (phéromones et choix de déplacement).

Étapes :
- [ ] Introduire un accès sûr aux phéromones avec indices signés (ex: `pheronome::at(int i, int j)` gérant i/j = -1..dim).
- [ ] Remplacer les appels dangereux dans `src/ant.cpp` (calcul `max_phen` et choix direction) par l'accès sûr.
- [ ] Remplacer les accès voisins dans `src/pheronome.hpp::mark_pheronome` par l'accès sûr.
- [ ] Ajouter un test edge-case : mise à jour phéromone sur une cellule en bord (i=0 ou j=0) ne crashe pas et donne une valeur attendue.

Fichiers / modules ciblés :
- `src/pheronome.hpp`
- `src/ant.cpp`
- `tests/test_pheromone_edges.cpp` (nouveau)

Dépendances :
- D1-01 (seed OK pour tests reproductibles).

Critères d'acceptation :
- Aucun accès hors limites sur bords (vérifiable via run debug/ASAN si disponible, sinon via tests ciblés).
- Tests edge-case passent.

Livrable / Preuve :
- Test edge-case vert + note de revue expliquant la stratégie de gestion des cellules fantômes.

[Tâche D1-03] — Représentation types (nid/nourriture/indésirable/libre) + multi-nourriture

Priorité : P0

Estimation : 7 h

Exigences liées : FR-01, FR-02, FR-06, FR-10, FR-13 ; NFR-07

Description :
Rendre explicite dans le code la notion de type de cellule (nid, nourriture, indésirable, libre) et permettre plusieurs sources de nourriture. Conserver une approche minimaliste (pas de UI avancée), mais testable.

Étapes :
- [ ] Ajouter une représentation explicite de la/les cellule(s) nourriture (ex: `std::vector<position_t> foods`).
- [ ] Mettre à jour la logique ant : une fourmi devient "chargée" si elle atteint n'importe quelle cellule nourriture.
- [ ] Mettre à jour la logique phéromones : garantir `V1=1` sur toutes les cellules nourriture et `V2=1` sur la cellule nid après `pheronome::update()`.
- [ ] Ajouter un mécanisme minimal pour cellules indésirables internes (ex: liste de positions marquées comme indésirables au démarrage).
- [ ] Ajouter un test : deux sources nourriture -> une fourmi qui atteint l'une ou l'autre devient "chargée" et le compteur peut s'incrémenter au nid.

Fichiers / modules ciblés :
- `src/ant.hpp`, `src/ant.cpp` (signature/logic si nécessaire)
- `src/pheronome.hpp`
- `src/ant_simu.cpp` (définition foods + indésirables)
- `src/renderer.cpp` (optionnel: affichage multi-food)
- `tests/test_multi_food.cpp` (nouveau)

Dépendances :
- D1-02 (accès voisins sûr pour éviter UB pendant tests).

Critères d'acceptation :
- Le code supporte 1..N cellules nourriture sans duplication de logique.
- Les cellules indésirables sont effectivement non traversables (testable).
- Les tests liés passent.

Livrable / Preuve :
- Test multi-food vert + démonstration (log) sur un scénario simple.

[Tâche D1-04] — Robustesse budget de mouvement (coût=0, bornes)

Priorité : P0

Estimation : 3 h

Exigences liées : FR-14 ; NFR-06

Description :
Empêcher toute boucle infinie dans `ant::advance` lorsque des cellules ont un coût nul, tout en respectant l'idée "budget de mouvement = 1 consommé selon le coût".

Étapes :
- [ ] Ajouter une borne dure sur le nombre de déplacements par pas de temps (constante/documentée).
- [ ] Couvrir le cas coût=0 (la borne doit empêcher l'infini).
- [ ] Ajouter un test/smoke ciblé: environnement avec au moins une cellule de coût 0 -> l'itération termine.

Fichiers / modules ciblés :
- `src/ant.cpp`
- `tests/test_movement_budget.cpp` (nouveau) ou scénario smoke

Dépendances :
- D1-02 (sécurité bords) recommandé avant d'augmenter couverture de tests.

Critères d'acceptation :
- `ant::advance` termine toujours en temps borné.
- Aucune boucle infinie observée en smoke run (exécution bornée).

Livrable / Preuve :
- Test/smoke vert + note (valeur de la borne et justification).

[Tâche D1-05] — Mettre en place harness tests + unit tests phéromones/évaporation

Priorité : P0

Estimation : 6 h

Exigences liées : FR-09, FR-10, FR-11, FR-15 ; NFR-05, NFR-07

Description :
Créer une base de tests automatisables sans dépendance externe lourde (runner minimal) et valider la mise à jour des phéromones et l'évaporation.

Étapes :
- [ ] Créer un runner minimal (ex: `tests/main.cpp` + `assert` + retour non-0).
- [ ] Ajouter un test unitaire : `mark_pheronome` applique `alpha*max + (1-alpha)*avg` (valeurs contrôlées).
- [ ] Ajouter un test unitaire : `update()` force V1=1 sur nourriture et V2=1 sur nid (après swap).
- [ ] Ajouter un test unitaire : `do_evaporation()` multiplie V1/V2 par beta sur cellules internes.
- [ ] Intégrer la compilation/exécution via `make test` (coordination avec J1-01).

Fichiers / modules ciblés :
- `tests/main.cpp` (nouveau)
- `tests/test_pheromone.cpp` (nouveau)
- `tests/test_evaporation.cpp` (nouveau)
- `src/pheronome.hpp`
- `src/Makefile` (via J1-01)

Dépendances :
- D1-02 (accès voisins sûr) pour que les tests couvrent aussi les bords.

Critères d'acceptation :
- Tests unitaires passent de manière déterministe.
- Les tests couvrent au moins 1 cas bord + 1 cas interne.

Livrable / Preuve :
- Sortie `make test` (log) + fichiers de tests commités.

[Tâche D1-06] — Tests d'intégration: états fourmi + compteur nourriture

Priorité : P1

Estimation : 4 h

Exigences liées : FR-05, FR-06, FR-07, FR-08 ; NFR-05, NFR-06

Description :
Valider la chaîne complète "déplacement -> prise nourriture -> retour nid -> incrément compteur" dans un scénario déterministe, sans UI.

Étapes :
- [ ] Construire un micro-scénario (petite grille, positions fixes, epsilon contrôlé).
- [ ] Exécuter un nombre borné d'itérations.
- [ ] Vérifier : passage à l'état chargé sur nourriture.
- [ ] Vérifier : retour au nid -> état non chargé.
- [ ] Vérifier : incrément du compteur uniquement quand la fourmi est chargée.
- [ ] Enregistrer le résultat attendu et le rendre testable (asserts).

Fichiers / modules ciblés :
- `tests/test_integration_food_counter.cpp` (nouveau)
- `src/ant.hpp`, `src/ant.cpp`
- `src/pheronome.hpp`
- `src/fractal_land.hpp` (si besoin d'un land minimal/mock)

Dépendances :
- D1-03 (multi-food/type explicite) si le test porte sur plusieurs nourritures.
- D1-05 (runner tests).

Critères d'acceptation :
- Test d'intégration passe de façon déterministe.
- Le compteur nourriture évolue comme attendu.

Livrable / Preuve :
- Test vert (`make test`) + description du scénario dans le commentaire de PR.

[Tâche D1-07] — Test normalisation paysage dans [0,1]

Priorité : P1

Estimation : 2 h

Exigences liées : FR-18 ; NFR-06

Description :
Rendre vérifiable que la normalisation du paysage fractal produit bien des valeurs dans [0,1] et ne génère pas de NaN/Inf.

Étapes :
- [ ] Écrire un test qui génère un `fractal_land` avec paramètres fixes.
- [ ] Appliquer la normalisation (comme dans `src/ant_simu.cpp`).
- [ ] Vérifier : min >= 0 et max <= 1.
- [ ] Vérifier : pas de NaN/Inf.
- [ ] Corriger si nécessaire l'initialisation de min/max pour rendre le calcul robuste.

Fichiers / modules ciblés :
- `src/fractal_land.hpp`, `src/fractal_land.cpp`
- `src/ant_simu.cpp` (normalisation)
- `tests/test_land_normalization.cpp` (nouveau)

Dépendances :
- D1-05 (runner tests).

Critères d'acceptation :
- Test vert et déterministe.

Livrable / Preuve :
- Test vert + log `make test`.

[Tâche D1-08] — Doc d'usage (paramètres + graines) + validation finale Daniel

Priorité : P1

Estimation : 3 h

Exigences liées : NFR-04, NFR-07

Description :
Mettre à jour la doc d'usage pour refléter le comportement It1 et fournir une preuve de validation (logs tests + smoke).

Étapes :
- [ ] Mettre à jour `README.md` : paramètres (m, alpha, beta, epsilon) et où les modifier/configurer.
- [ ] Mettre à jour `README.md` : graines (land + fourmis) et impact reproductibilité.
- [ ] Mettre à jour `README.md` : comment lancer `make test` et `make smoke`.
- [ ] Lancer `make test` et `make smoke`, archiver la sortie (à coller dans PR ou note de merge).
- [ ] Faire un passage de validation sur les critères d'acceptation It1 (iterations.md).

Fichiers / modules ciblés :
- `README.md`
- `src/ant_simu.cpp` (paramètres/graines, si clarification nécessaire)

Dépendances :
- D1-05..D1-07 (tests en place).
- J1-01 si les cibles Makefile doivent être documentées.

Critères d'acceptation :
- README cohérent avec le code, instructions exécutables.
- Logs de validation disponibles.

Livrable / Preuve :
- `README.md` mis à jour + sortie `make test`/`make smoke` archivée.

## Dépendances & synchronisation
- Dépendance d'itération : Itération 0 terminée (toolchain OK).
- Point de synchro quotidien (15 min) : état des correctifs critiques + état des tests.
- Revue Javier en continu (au fil des PR).
- Intégration intermédiaire le 12/02 (merge partiel si stable) pour réduire risque de conflits.
- Feature freeze le 14/02 midi, puis J1-03 intégration finale.

## Risques & mitigations
- Accès mémoire hors limites (indices signed/unsigned, bords) -> Mitigation : D1-02 + tests edges + build debug si possible.
- Boucle infinie (coût=0) -> Mitigation : D1-04 + smoke test borné.
- Tests difficiles à écrire (dépendance SDL / architecture) -> Mitigation : runner minimal sans UI, scénarios déterministes, éviter la fenêtre SDL.
- Scope creep (vectorisation/mesures perf) -> Mitigation : refuser tout ajout perf, report à It2.

## Definition of Done (Itération 1)
- [ ] Tous les tests unitaires et d'intégration passent via `make test`.
- [ ] Smoke run (`make smoke`) termine et montre un comportement plausible (aucun crash/boucle infinie).
- [ ] Aucun déplacement sur cellule indésirable, aucune sortie de grille observée sur scénario smoke.
- [ ] Normalisation du paysage vérifiée dans [0,1] (test vert).
- [ ] Documentation `README.md` mise à jour (paramètres + graines + commandes).
- [ ] Intégration réalisée (J1-03) et validation finale signée (J1-04).
