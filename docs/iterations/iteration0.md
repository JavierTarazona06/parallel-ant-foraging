# Itération 0 — Démarrage & cadrage (08–09/02)

## Objectifs & livrables
- Toolchain validée : compilation C++17 + SDL2 + OpenMP sur l’environnement cible.
- Build reproductible en local en modes debug et release (ou équivalent).
- Documentation d’exécution : commandes build/run + paramètres par défaut + graines (paysage + fourmis).
- Harness de mesure (squelette) : conventions d’exécution, format CSV, scripts prêts à être complétés.
- Backlog détaillé par exigence (FR/NFR) avec estimations, dépendances et preuves attendues.
- Intégration des changements It0 + vérification (build + exécution minimale).

## Exigences ciblées (FR / NFR)
- NFR-04 (reproductibilité contrôlée : graines)
- NFR-07 (maintenabilité : séparation outillage/mesures)
- NFR-01 (préparation : protocole de mesure comparable)

## Plan de travail — Javier
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| J0-01 | Valider toolchain + cibles debug/release | P0 | 6 | — | Builds debug/release OK + `src/Makefile` ajusté |
| J0-02 | Backlog FR/NFR + hypothèses restantes | P0 | 6 | — | `docs/iterations/backlog.md` validé par l’équipe |
| J0-03 | Intégration (merge) + vérification It0 | P0 | 3 | J0-01, J0-02, D0-01..D0-04 | Merge + logs build/run + check DoD |
| J0-04 | Validation finale It0 (DoD) + go/no-go | P0 | 1 | J0-03 | Checklist DoD complétée |

[Tâche J0-01] — Valider toolchain + cibles debug/release

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-07 (et support de NFR-04 via exécution reproductible)

Description :
Valider la chaîne de compilation/exécution sur l’environnement cible (C++17, SDL2, OpenMP) et rendre les builds debug/release simples et reproductibles pour réduire les frictions sur les itérations suivantes.

Étapes :
- [ ] Vérifier versions et dépendances : compilateur C++17, headers/libs SDL2, support OpenMP.
- [ ] Compiler en mode release (commande existante) et vérifier la production de `src/ant_simu.exe`.
- [ ] Compiler en mode debug (ex. `DEBUG=yes`) et vérifier l’exécution sans crash immédiat.
- [ ] Si nécessaire, ajouter des cibles `debug` et `release` (wrappers) dans `src/Makefile` sans changer la logique de compilation.
- [ ] Documenter les commandes exactes (copier/coller) et les prérequis SDL2 dans `README.md`.
- [ ] Archiver un log minimal de compilation (console) pour preuve de reproductibilité.

Fichiers / modules ciblés :
- `src/Makefile`
- `src/Make_linux.inc`, `src/Make_msys2.inc`, `src/Make_osx.inc` (si ajustements requis)
- `README.md`

Dépendances :
- Aucune.

Critères d’acceptation :
- Les builds debug et release compilent sans erreur sur la machine de dev.
- Les commandes sont documentées et fonctionnent sans étape implicite.
- Le binaire se lance et peut être fermé proprement (validation minimale).

Livrable / Preuve :
- `src/Makefile` (cibles et/ou help mis à jour) + sortie console de build (à coller dans PR ou note).

[Tâche J0-02] — Backlog FR/NFR + hypothèses restantes

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-07 (structuration) ; NFR-01 (préparation) ; NFR-04 (préparation)

Description :
Transformer `docs/conception.md` en backlog actionnable par exigence (tâches, estimations, dépendances, preuves), et clarifier les hypothèses encore ouvertes avant de lancer l’implémentation lourde.

Étapes :
- [ ] Extraire la liste des exigences FR-xx et NFR-xx depuis `docs/conception.md`.
- [ ] Créer `docs/iterations/backlog.md` avec, pour chaque exigence : objectif, tâches concrètes, propriétaire pressenti, estimation, dépendances, preuve attendue (test/mesure/doc).
- [ ] Lister les hypothèses ouvertes (ex. protocole de mesure, exécution sans UI, techno multi-processus) et décider ce qui doit être tranché en It1/It2.
- [ ] Vérifier la cohérence avec `docs/iterations/iterations.md` (portée et dates).
- [ ] Revue croisée avec Daniel et ajustement des estimations.

Fichiers / modules ciblés :
- `docs/conception.md`
- `docs/iterations/iterations.md` (lecture/référence)
- `docs/iterations/backlog.md` (nouveau)

Dépendances :
- Aucune.

Critères d’acceptation :
- Chaque FR/NFR a au moins une tâche associée et une preuve attendue.
- Les hypothèses restantes sont listées clairement avec un owner et une échéance de décision.
- Le backlog est jugé “prêt à exécuter” par l’équipe (revue 15 min).

Livrable / Preuve :
- `docs/iterations/backlog.md` rempli + note de validation (date + participants) en tête du fichier.

[Tâche J0-03] — Intégration (merge) + vérification It0

Priorité : P0

Estimation : 3 h

Exigences liées : NFR-07 (discipline d’intégration) ; NFR-04 (reproductibilité documentée)

Description :
Intégrer les changements It0 (Makefile, docs, scripts) et vérifier qu’un tiers peut compiler et exécuter le projet avec les informations fournies.

Étapes :
- [ ] Regrouper les changements It0 (docs + outillage) dans une PR/branche intégrable.
- [ ] Vérifier qu’aucun changement ne modifie la logique de simulation (It0 = cadrage/outillage).
- [ ] Merger et exécuter les commandes documentées : build debug + build release.
- [ ] Lancer une exécution minimale (ouverture/fermeture) et archiver un log court.
- [ ] Vérifier la présence et la cohérence : `README.md`, `scripts/` (si ajoutés), `docs/iterations/backlog.md`.

Fichiers / modules ciblés :
- `src/Makefile`
- `README.md`
- `scripts/*` (si ajoutés)
- `docs/iterations/backlog.md`

Dépendances :
- J0-01, J0-02 terminées.
- D0-01..D0-04 terminées.

Critères d’acceptation :
- Les builds debug/release passent après merge.
- Les instructions de `README.md` suffisent pour compiler et lancer.
- Les livrables It0 sont présents et cohérents.

Livrable / Preuve :
- Merge réalisé + logs de build/run (collés dans la description de merge ou note de validation).

[Tâche J0-04] — Validation finale It0 (DoD) + go/no-go

Priorité : P0

Estimation : 1 h

Exigences liées : NFR-07

Description :
Valider formellement la Definition of Done It0 et acter le passage à l’Itération 1.

Étapes :
- [ ] Parcourir la checklist DoD It0 avec Daniel.
- [ ] Pointer les écarts et décider : corriger immédiatement vs créer une tâche prioritaire It1.
- [ ] Émettre un go/no-go explicite.

Fichiers / modules ciblés :
- `docs/iterations/iteration0.md`
- `docs/iterations/backlog.md`

Dépendances :
- J0-03 terminé.

Critères d’acceptation :
- Toutes les cases DoD sont cochées ou explicitement reportées avec justification.
- Go/no-go annoncé et consigné (date).

Livrable / Preuve :
- Checklist DoD cochée + note “Go It1” (date + participants) ajoutée en fin de fichier.

## Plan de travail — Daniel
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| D0-01 | Squelette harness mesures (chrono + scripts) | P0 | 6 | J0-01 (commande build/run confirmée) | `scripts/bench/*` + schéma CSV |
| D0-02 | Documentation run : paramètres + graines | P0 | 4 | J0-01, D0-03 | `README.md` complété (build/run/seed) |
| D0-03 | Tests/validation minimale : build+run + logs | P0 | 4 | J0-01 | Logs debug/release + note smoke |
| D0-04 | Relecture backlog + protocole de mesures | P1 | 2 | J0-02, D0-01 | Commentaires + décisions gelées |

[Tâche D0-01] — Squelette harness mesures (chrono + scripts)

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-01 (préparation) ; NFR-04 ; NFR-07

Description :
Esquisser un harness de mesure combinant (i) conventions de mesure (quoi mesurer, comment comparer) et (ii) scripts/artefacts minimums (même stubs) pour préparer l’instrumentation fine des itérations suivantes.

Étapes :
- [ ] Créer un répertoire `scripts/bench/` (ou équivalent) pour isoler l’outillage de mesure.
- [ ] Rédiger `scripts/bench/README.md` : protocole minimal (mêmes paramètres + mêmes graines + même machine) et règles de comparaison.
- [ ] Définir un schéma CSV “v0” (entête + colonnes obligatoires) dans `scripts/bench/schema.csv` (ou doc) :
  - variant (ref/soa/omp/dist1/dist2), threads, processes
  - seed_land, seed_ants, m, alpha, beta, eps, dim, iterations
  - t_wall_ms (et colonnes réservées optionnelles pour temps par module)
- [ ] Écrire un script squelette `scripts/bench/run_bench.sh` qui :
  - vérifie la présence du binaire
  - affiche les TODO si le mode “headless/itérations bornées” n’est pas encore disponible
  - prépare un fichier CSV avec l’entête
- [ ] Valider avec Javier que le format CSV est compatible avec les tableaux d’accélération attendus en It2/It3.

Fichiers / modules ciblés :
- `scripts/bench/README.md` (nouveau)
- `scripts/bench/schema.csv` (nouveau)
- `scripts/bench/run_bench.sh` (nouveau)
- `docs/iterations/iterations.md` (lecture/référence pour critères de mesure)

Dépendances :
- J0-01 (pour confirmer le chemin/nom du binaire et la commande de build/run).

Critères d’acceptation :
- Le schéma CSV est défini et versionné (colonnes stables).
- Le script squelette s’exécute et génère au minimum un CSV avec entête, même si certaines étapes sont TODO.
- Le protocole de comparaison est explicitement écrit (mêmes paramètres/graines).

Livrable / Preuve :
- `scripts/bench/*` présents + exemple de CSV généré (même vide, entête uniquement).

[Tâche D0-02] — Documentation run : paramètres + graines

Priorité : P0

Estimation : 4 h

Exigences liées : NFR-04 ; NFR-07

Description :
Documenter précisément comment lancer la simulation et quels paramètres/graines sont utilisés par défaut afin de rendre les exécutions comparables et reproductibles.

Étapes :
- [ ] Ajouter dans `README.md` une section “Build” (pré-requis SDL2 + commandes debug/release).
- [ ] Ajouter une section “Run (référence)” avec la commande pour lancer `src/ant_simu.exe` et fermer proprement.
- [ ] Extraire depuis le code les valeurs par défaut (au minimum : graines, m, alpha, beta, eps, dimension) et les documenter.
- [ ] Documenter explicitement où les graines sont fixées dans le code (référence fichier/symbole).
- [ ] Vérifier la cohérence doc vs code via une exécution minimale (lien vers D0-03).

Fichiers / modules ciblés :
- `README.md`
- `src/ant_simu.cpp` (référence des paramètres/graines)

Dépendances :
- J0-01 (commandes build confirmées).
- D0-03 (validation run pour s’assurer que la doc est exécutable).

Critères d’acceptation :
- Un lecteur peut compiler et lancer en suivant uniquement `README.md`.
- Les graines et paramètres par défaut sont listés et traçables au code.

Livrable / Preuve :
- `README.md` mis à jour + commande de run validée (log court).

[Tâche D0-03] — Tests/validation minimale : build+run + logs

Priorité : P0

Estimation : 4 h

Exigences liées : NFR-07 (discipline de validation) ; support NFR-04

Description :
Exécuter une validation minimale (compilation + lancement) pour s’assurer que la base est saine avant l’Itération 1.

Étapes :
- [ ] Compiler en release et en debug selon les commandes It0 (ou nouvelles cibles).
- [ ] Lancer le binaire et vérifier : fenêtre SDL s’ouvre, aucun crash immédiat, fermeture propre.
- [ ] Capturer un log court (build + run) et le déposer comme preuve (PR/notes).
- [ ] Noter tout blocage (SDL manquant, flags, droits) dans `docs/iterations/backlog.md` (section “Blocages It0”).

Fichiers / modules ciblés :
- `src/Makefile`
- `src/ant_simu.cpp` (si besoin d’identifier un paramètre/log)
- `docs/iterations/backlog.md` (si blocages détectés)

Dépendances :
- J0-01 (toolchain validée / commande build).

Critères d’acceptation :
- Build debug et release réussis.
- Exécution minimale réussie et reproductible (même machine, même binaire).
- Les logs de preuve sont disponibles.

Livrable / Preuve :
- Logs compilation/exécution (collés dans PR ou note d’itération) + liste des blocages (si présents).

[Tâche D0-04] — Relecture backlog + protocole de mesures

Priorité : P1

Estimation : 2 h

Exigences liées : NFR-01 (préparation) ; NFR-07

Description :
Relire le backlog et le protocole de mesure pour éviter les angles morts (mesures non comparables, paramètres non tracés, tâches manquantes).

Étapes :
- [ ] Relire `docs/iterations/backlog.md` et commenter (estimations, dépendances, preuves).
- [ ] Vérifier que le schéma CSV v0 couvre bien les champs nécessaires aux tableaux d’accélération.
- [ ] Proposer des ajustements minimaux (sans ajouter de nouvelles fonctionnalités hors scope).
- [ ] Valider avec Javier les décisions gelées pour It1 (format seeds/params, conventions bench).

Fichiers / modules ciblés :
- `docs/iterations/backlog.md`
- `scripts/bench/schema.csv`
- `docs/iterations/iterations.md` (lecture/référence)

Dépendances :
- J0-02 (backlog initial).
- D0-01 (schéma/protocole de mesures).

Critères d’acceptation :
- Backlog et schéma CSV sont cohérents et suffisamment précis pour démarrer It1/It2.
- Les décisions prises sont listées explicitement (avec date).

Livrable / Preuve :
- Commentaires intégrés (ou section “Décisions gelées” ajoutée) dans `docs/iterations/backlog.md`.

## Dépendances & synchronisation
- Jour 1 (08/02) matin : J0-01 et D0-01 en parallèle.
- Point de synchro (08/02) midi (15 min) : statut build + choix emplacement scripts + schéma CSV v0.
- Jour 1 (08/02) fin de journée : D0-03 (validation minimale) + premiers retours README.
- Jour 2 (09/02) matin : J0-02 (backlog) + D0-02 (doc) en parallèle.
- Point de synchro (09/02) après-midi (30 min) : revue croisée backlog/protocole (D0-04) + décisions gelées.
- Fin It0 (09/02) : J0-03 intégration + J0-04 go/no-go.

## Risques & mitigations
- SDL2 manquant ou incompatible -> Mitigation : documenter l’installation, valider sur une machine de référence, fournir commandes de diagnostic.
- Programme non automatisable (boucle SDL non bornée) pour les mesures -> Mitigation : garder un harness “squelette” (stubs) et planifier un mode d’exécution borné en It1/It2 (décision consignée au backlog).
- Incohérence doc vs code (paramètres/graines hardcodés) -> Mitigation : référencer précisément les constantes (fichier + lignes) et ajouter une vérification smoke (D0-03).
- Divergence sur les hypothèses distribuées (MPI ou autre) -> Mitigation : lister l’option retenue comme hypothèse à valider en It2/It3, sans bloquer It1.

## Definition of Done (Itération 0)
- [ ] Toolchain validée : compilation C++17 + SDL2 + OpenMP confirmée.
- [ ] Builds debug et release reproductibles, commandes documentées.
- [ ] `README.md` contient : prérequis, build, run, paramètres/graines par défaut.
- [ ] Harness de mesure “squelette” présent (`scripts/bench/*`) + schéma CSV v0 défini.
- [ ] Backlog FR/NFR produit avec estimations, dépendances et preuves attendues (`docs/iterations/backlog.md`).
- [ ] Validation minimale effectuée (build+run) et logs disponibles.
- [ ] Intégration réalisée (merge) et go/no-go acté pour l’Itération 1.
