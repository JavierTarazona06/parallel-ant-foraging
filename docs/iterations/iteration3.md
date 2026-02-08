# Itération 3 — Distribué, stabilisation & livraison (22–27/02)

## Objectifs & livrables
- Deux stratégies distribuées opérationnelles sur au moins 2 processus :
  - Méthode 1 : environnement complet par processus + contrôle d’un sous-ensemble de fourmis + fusion des phéromones par maximum.
  - Méthode 2 : découpage spatial + échanges de bords (halo) + migration des fourmis.
- Scripts d’exécution multi-processus (paramètres et graines reproductibles).
- Campagnes de mesure (p=1,2,4,...) et tableaux d’accélération vs nombre de processus (un cœur par processus).
- Stabilisation avant rendu : tests de régression verts, correction des bugs critiques, gel le 27/02.
- Documentation finale : comment compiler/exécuter (séquentiel, OpenMP, distribué), protocole de mesure, résultats et limites connues.

## Exigences ciblées (FR / NFR)
- FR-22, FR-23
- NFR-03, NFR-04, NFR-05, NFR-06, NFR-07, NFR-08

## Plan de travail — Javier
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| J3-01 | Revue architecture distribué + décisions (méthodes 1/2) | P0 | 5 | D3-01 (draft) | Note d’archi validée + checklist NFR-07/NFR-08 |
| J3-02 | Build/packaging: cibles MPI + exécution headless | P0 | 6 | D3-02 (proto) | `make dist` / doc compilation + run sur 2 processus |
| J3-03 | Validation QA: plan de régression + critères d’acceptation | P0 | 6 | D3-02..D3-04, D3-06 | Rapport QA (tests + runs) + liste écarts |
| J3-04 | Intégration (merge) + vérification bout-en-bout | P0 | 4 | D3-02..D3-07 | Merge + logs build/tests/bench + check DoD |
| J3-05 | Stabilisation buffer (25–26/02) + gel (27/02) | P0 | 6 | J3-04 | Checklist “prêt à rendre” + aucun bloquant |

[Tâche J3-01] — Revue architecture distribué + décisions (méthodes 1/2)

Priorité : P0

Estimation : 5 h

Exigences liées : FR-22, FR-23 ; NFR-07, NFR-08, NFR-05

Description :
Valider le design technique des deux stratégies distribuées (données échangées, fréquence des échanges, cohérence phéromones, gestion des bords) pour limiter les refactors tardifs et garder une base maintenable/portable.

Étapes :
- [ ] Lire le plan d’implémentation Daniel (D3-01) et vérifier l’alignement avec `docs/conception.md`.
- [ ] Valider la stratégie de communication (MPI si disponible, sinon fallback documenté) et la forme des échanges.
- [ ] Verrouiller les invariants à préserver en distribué (phéromones, compteur, bords, indésirables).
- [ ] Confirmer la définition des mesures distribué : temps global = max des temps processus ; speedup = T1/Tp.

Fichiers / modules ciblés :
- Zones logiques : “distributed runtime”, “exchange layer”, “method1”, “method2”
- `docs/conception.md` (lecture/validation)
- `docs/iterations/iteration3.md` (ce document, si besoin d’ajustements mineurs)

Dépendances :
- D3-01 (draft d’archi/choix techniques).

Critères d’acceptation :
- Décisions explicites : quoi est échangé, quand, et comment est garantie la cohérence.
- Aucun point bloquant non traité (ou plan de mitigation documenté).

Livrable / Preuve :
- Note courte (PR description) + checklist de décisions signée.

[Tâche J3-02] — Build/packaging: cibles MPI + exécution headless

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-08 ; support de FR-22/FR-23

Description :
Rendre compilable et exécutable la variante distribuée (sans dépendre du rendu SDL) via une cible dédiée, pour faciliter la CI locale et les mesures.

Étapes :
- [ ] Ajouter une cible de build dédiée au distribué (ex: `ant_simu_dist.exe`) avec les flags nécessaires.
- [ ] Vérifier l’exécution “headless” (sans fenêtre) pour les runs de mesure.
- [ ] Documenter les commandes (mpirun, variables, paramètres) et les prérequis.

Fichiers / modules ciblés :
- `src/Makefile`
- `README.md` (ou doc dédiée en `docs/`)
- `src/ant_simu.cpp` (ou nouveau `src/ant_simu_dist.cpp`)

Dépendances :
- D3-02 (proto méthode 1) pour valider la chaîne build/run.

Critères d’acceptation :
- Compilation OK et exécution sur 2 processus avec un scénario court.
- Instructions de build/run reproductibles sur l’environnement cible.

Livrable / Preuve :
- Logs `make ...` + logs d’exécution `mpirun -np 2 ...` (ou équivalent).

[Tâche J3-03] — Validation QA: plan de régression + critères d’acceptation

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-05, NFR-06, NFR-07 ; FR-22, FR-23

Description :
Définir et exécuter une validation minimale mais fiable des deux méthodes distribuées : invariants, stabilité, absence de crash, cohérence des phéromones et reproductibilité.

Étapes :
- [ ] Établir une matrice de tests : (mode ref / OpenMP / dist m1 / dist m2) × (p=1,2,4).
- [ ] Définir des invariants vérifiables automatiquement (pas de sortie de grille, pas sur indésirable, phéromones bornées, V1/V2 forcés sur food/nest, compteur monotone).
- [ ] Rejouer `make test` et les tests de non-régression It2 ; ajouter un “smoke distribué” si nécessaire.
- [ ] Documenter les résultats et écarts acceptés (si non déterminisme en distribué).

Fichiers / modules ciblés :
- `tests/*`
- `scripts/*` (runs de smoke/bench)
- `docs/` (rapport QA court)

Dépendances :
- D3-02..D3-04 (deux méthodes fonctionnelles).
- D3-06 (scripts de bench disponibles) utile mais non bloquant pour le smoke.

Critères d’acceptation :
- Aucun crash sur p=2 pour les deux méthodes.
- Invariants validés automatiquement sur au moins un scénario fixe.
- Écarts documentés et justifiés.

Livrable / Preuve :
- Log `make test` + logs de smoke distribué + note QA.

[Tâche J3-04] — Intégration (merge) + vérification bout-en-bout

Priorité : P0

Estimation : 4 h

Exigences liées : FR-22, FR-23 ; NFR-03, NFR-05, NFR-07, NFR-08

Description :
Intégrer les changements It3 et vérifier que build/tests/bench passent. Assurer que les tableaux d’accélération processus sont publiés avec le protocole.

Étapes :
- [ ] Feature freeze It3 le 27/02 10:00.
- [ ] Merger dans un ordre limitant les conflits (méthode 1 -> scripts -> méthode 2 -> docs/bench).
- [ ] Lancer `make clean && make all` et `make test`.
- [ ] Lancer la campagne bench processus (D3-06) et vérifier la génération du tableau speedup.
- [ ] Vérifier que la doc finale pointe vers les commandes et artefacts.

Fichiers / modules ciblés :
- Branche principale (intégration)
- `src/*`, `tests/*`, `scripts/*`, `docs/*`

Dépendances :
- D3-02..D3-07 terminées ou “prêtes à merger”.

Critères d’acceptation :
- Build OK + tests OK.
- Tableaux speedup processus présents (au moins p=1,2) et protocole documenté.

Livrable / Preuve :
- Commit(s) de merge + logs build/tests/bench + liens vers docs.

[Tâche J3-05] — Stabilisation buffer (25–26/02) + gel (27/02)

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-06 ; critère de livraison It3

Description :
Utiliser explicitement le buffer pour corriger les bugs critiques, stabiliser, et finaliser la version à rendre.

Étapes :
- [ ] Trier les bugs par sévérité (bloquant / majeur / mineur).
- [ ] Corriger uniquement les bloquants/majeurs (pas de refactor perf).
- [ ] Rejouer la validation QA (J3-03) après chaque fix critique.
- [ ] Geler la version finale le 27/02 (aucun changement sans raison critique).

Fichiers / modules ciblés :
- Modules modifiés It3
- `docs/*` (notes de release si nécessaire)

Dépendances :
- J3-04 (intégration réalisée).

Critères d’acceptation :
- Aucun ticket bloquant ouvert le 27/02.
- Résultats et doc cohérents avec la version gelée.

Livrable / Preuve :
- Checklist “prêt à rendre” complétée + note de gel.

## Plan de travail — Daniel
| ID tâche | Titre | Priorité | Estimation (h) | Dépendances | Livrable/Preuve |
|---|---|---|---:|---|---|
| D3-01 | Spécification technique distribué (API + échanges) | P0 | 4 | It2 DoD | Plan technique (méthode 1/2) + interfaces |
| D3-02 | Implémenter distribué — Méthode 1 (carte complète + max) | P0 | 12 | D3-01 | Run sur 2 processus + fusion phéromones validée |
| D3-03 | Implémenter distribué — Méthode 2 (découpage spatial + halos) | P0 | 14 | D3-01 | Run sur 2 processus + migration fourmis OK |
| D3-04 | Scripts exécution multi-processus (reproductible) | P0 | 6 | D3-02 | Script(s) `mpirun` + paramètres/gaines figés |
| D3-05 | Tests/validation distribuée (invariants + non-régression) | P0 | 8 | D3-02, D3-03 | Tests verts + smoke distribué |
| D3-06 | Campagnes de mesure processus + tableaux speedup | P0 | 8 | D3-04, D3-05 | Tableau accélération processus (Markdown) + CSV |
| D3-07 | Documentation finale (how-to, résultats, limites) | P1 | 6 | D3-06 | Doc finale + liens vers scripts/artefacts |

[Tâche D3-01] — Spécification technique distribué (API + échanges)

Priorité : P0

Estimation : 4 h

Exigences liées : FR-22, FR-23 ; NFR-07, NFR-08

Description :
Définir précisément les interfaces et le protocole d’échanges pour implémenter les deux méthodes distribuées sans ambiguïté (quels buffers, quelle fréquence, quelles réductions), en s’appuyant sur l’instrumentation It2.

Étapes :
- [ ] Choisir la couche de communication (MPI recommandé si disponible ; sinon fallback “multi-process local” documenté).
- [ ] Définir les buffers échangés (phéromones V1/V2, halos, migrations de fourmis, métriques).
- [ ] Définir la granularité de synchronisation par itération (quand fusion max/échanges halos).
- [ ] Définir le format de sortie (CSV) côté distribué et la règle d’agrégation (temps global = max des rangs).

Fichiers / modules ciblés :
- Zones logiques : `src/distributed/*`, `src/sim_step.*`, `src/timing.*`
- `scripts/*` (appel/paramètres)

Dépendances :
- It2 DoD (instrumentation + headless + CSV).

Critères d’acceptation :
- Design “decision complete” : pas de point majeur laissé implicite.

Livrable / Preuve :
- Document de design court (dans la description d’une PR) + schéma d’échanges (texte).

[Tâche D3-02] — Implémenter distribué — Méthode 1 (carte complète + max)

Priorité : P0

Estimation : 12 h

Exigences liées : FR-22 ; NFR-03, NFR-04, NFR-05, NFR-08

Description :
Implémenter la méthode 1 : chaque processus possède l’environnement complet et ne gère qu’une partie des fourmis. Lors de la phase d’évaporation/fusion, produire une carte globale cohérente en prenant le maximum des phéromones par cellule entre processus.

Étapes :
- [ ] Répartir les fourmis par rang (split stable : range d’indices).
- [ ] Exécuter les pas de simulation locaux (fourmis locales) sur la carte locale.
- [ ] Évaporation : partitionner la carte par rang (chaque rang évapore une tranche), préparer un buffer “contribution”.
- [ ] Fusion : réduire par maximum sur V1/V2 pour reconstruire la carte globale cohérente sur tous les rangs.
- [ ] Valider un scénario court : p=2, mêmes graines, fin sans crash et compteur plausible.

Fichiers / modules ciblés :
- Nouveau : `src/distributed/method1.*` (ou équivalent)
- `src/pheronome.hpp` (export/import buffers V1/V2 si nécessaire)
- `src/ant_simu.cpp` (ou `src/ant_simu_dist.cpp`) : mode distribué + paramètres
- `scripts/run_dist_method1.sh` (nouveau)

Dépendances :
- D3-01.

Critères d’acceptation :
- Exécution p=2 fonctionne et produit des métriques + CSV.
- Fusion “max” est appliquée sur V1/V2 (preuve via logs ou test ciblé).

Livrable / Preuve :
- Log d’exécution p=2 + extrait CSV + note décrivant la fusion max.

[Tâche D3-03] — Implémenter distribué — Méthode 2 (découpage spatial + halos)

Priorité : P0

Estimation : 14 h

Exigences liées : FR-23 ; NFR-03, NFR-04, NFR-05, NFR-06, NFR-08

Description :
Implémenter la méthode 2 : chaque processus gère une sous-carte, échange les halos aux bords et transfère les fourmis franchissant une frontière. Objectif minimal : fonctionner sur p=2 (décomposition 1D) avec cohérence des bords.

Étapes :
- [ ] Choisir une décomposition simple (1D par bandes verticales ou horizontales) et la figer.
- [ ] Définir halos d’épaisseur 1 cellule pour V1/V2 (car dépendance voisinage 4).
- [ ] Échanger les halos à chaque itération (ou au minimum avant les calculs de bords nécessaires).
- [ ] Détecter les fourmis sortant de la sous-carte et les migrer vers le rang voisin (envoi des champs nécessaires : position, état, seed).
- [ ] Ajouter un “guard” pour le déséquilibre de charge (au moins instrumentation : nombre de fourmis par rang).
- [ ] Valider scénario court p=2 : pas de deadlock, migration OK, invariants respectés.

Fichiers / modules ciblés :
- Nouveau : `src/distributed/method2.*`
- Nouveau : `src/distributed/partition.*` (helpers indices locaux/globaux, halos)
- `src/ants_soa.*` (si l’état des fourmis est en SOA)
- `src/pheronome.hpp` (accès halos / export/import bord)
- `scripts/run_dist_method2.sh` (nouveau)

Dépendances :
- D3-01.
- D3-02 recommandé avant (réutilisation de l’infra comm/CLI).

Critères d’acceptation :
- Exécution p=2 fonctionne (pas de deadlock) et respecte les invariants.
- Les échanges de bords sont cohérents (preuve via test bords ou log de contrôle).

Livrable / Preuve :
- Log p=2 + extrait CSV + note sur la décomposition et les échanges.

[Tâche D3-04] — Scripts exécution multi-processus (reproductible)

Priorité : P0

Estimation : 6 h

Exigences liées : NFR-04 ; support NFR-03

Description :
Fournir des scripts standard pour exécuter les méthodes distribuées avec paramètres et graines fixés, afin de rendre les mesures et la validation répétables.

Étapes :
- [ ] Définir un scénario “bench” figé (taille, m, alpha, beta, eps, seeds, nb_iter).
- [ ] Écrire des scripts `run_dist_method1.sh` / `run_dist_method2.sh` qui acceptent `-np` et exportent les CSV dans un dossier daté.
- [ ] Ajouter une convention de nommage des sorties (mode, p, date, commit).
- [ ] Documenter les commandes dans une section dédiée.

Fichiers / modules ciblés :
- `scripts/run_dist_method1.sh` (nouveau)
- `scripts/run_dist_method2.sh` (nouveau)
- `README.md` ou `docs/runbook.md` (nouveau, si préférable)

Dépendances :
- D3-02 (au moins la méthode 1 doit tourner).

Critères d’acceptation :
- Un run p=2 produit systématiquement un CSV exploitable.
- Les paramètres/graines utilisés sont visibles dans les logs/CSV.

Livrable / Preuve :
- Scripts commités + exemple de sortie CSV + commandes documentées.

[Tâche D3-05] — Tests/validation distribuée (invariants + non-régression)

Priorité : P0

Estimation : 8 h

Exigences liées : NFR-05, NFR-06 ; FR-22, FR-23

Description :
Mettre en place une validation automatique minimale pour détecter les régressions liées au distribué (bords, halos, migrations, fusion max), et confirmer la stabilité avant mesures.

Étapes :
- [ ] Définir les invariants (comme It2) et ajouter un test ou un mode “verify” qui les vérifie en fin de run.
- [ ] Ajouter un test ciblé “fusion max” (méthode 1) : deux rangs avec valeurs différentes -> max attendu.
- [ ] Ajouter un test ciblé “halo” (méthode 2) : cohérence sur une cellule bord après échange.
- [ ] Ajouter un smoke distribué dans la CI locale (ou `make smoke_dist`) si possible.

Fichiers / modules ciblés :
- `tests/*` (nouveaux tests)
- `src/distributed/*`
- `src/Makefile` (cibles smoke/test si besoin)

Dépendances :
- D3-02 et D3-03 (au moins un proto exécutable).

Critères d’acceptation :
- `make test` reste vert.
- Les deux méthodes passent un smoke p=2 avec vérification des invariants.

Livrable / Preuve :
- Log `make test` + log de smoke distribué (p=2) + description des invariants.

[Tâche D3-06] — Campagnes de mesure processus + tableaux speedup

Priorité : P0

Estimation : 8 h

Exigences liées : NFR-03 ; NFR-01 (protocole) ; FR-22/FR-23 (évaluation)

Description :
Exécuter des campagnes de mesures sur p processus (p=1,2,4,...) et produire les tableaux d’accélération. La mesure doit utiliser une définition univoque du temps global (max des rangs).

Étapes :
- [ ] Geler le scénario bench (mêmes paramètres et graines).
- [ ] Exécuter les runs pour p=1,2,4 (et plus si possible) pour méthode 1 et méthode 2.
- [ ] Agréger les CSV : temps global = max temps (par itération ou total) ; calculer speedup S(p)=T(1)/T(p).
- [ ] Générer un tableau Markdown : p, T(p), S(p) + notes (machine, coeurs, protocole).
- [ ] Répéter au moins 2 fois et reporter moyenne/mediane si le bruit est significatif.

Fichiers / modules ciblés :
- `scripts/bench_processes.sh` (nouveau)
- `scripts/summarize_speedup_processes.*` (nouveau)
- `docs/perf_iteration3.md` (nouveau) ou équivalent
- `docs/` (CSV archivés)

Dépendances :
- D3-04 (scripts run) et D3-05 (stabilité/invariants).

Critères d’acceptation :
- Tableau speedup processus produit pour au moins une méthode (priorité méthode 1) et p>=2.
- Protocole reproductible (paramètres/graines/commandes).

Livrable / Preuve :
- Tableaux Markdown + CSV + logs de commandes.

[Tâche D3-07] — Documentation finale (how-to, résultats, limites)

Priorité : P1

Estimation : 6 h

Exigences liées : NFR-07, NFR-08 ; FR-22, FR-23

Description :
Finaliser la documentation de livraison : exécution, paramètres, limites, et résultats de mesures (threads et processus).

Étapes :
- [ ] Mettre à jour la doc d’exécution : séquentiel, OpenMP, distribué (méthode 1/2).
- [ ] Ajouter le protocole de mesure et pointer vers les scripts.
- [ ] Insérer les tableaux d’accélération processus et observations (bottlenecks, échanges).
- [ ] Lister les limites connues (ex: déséquilibre méthode 2, dépendances MPI) et les contournements.

Fichiers / modules ciblés :
- `README.md` et/ou `docs/runbook.md`
- `docs/perf_iteration3.md` (ou équivalent)

Dépendances :
- D3-06.

Critères d’acceptation :
- Un lecteur peut compiler/exécuter les deux méthodes via les commandes documentées.
- Les résultats (tableaux) sont présents et traçables (paramètres/graines).

Livrable / Preuve :
- Doc finale commitée + liens vers scripts/artefacts.

## Dépendances & synchronisation
- Dépendance d’itération : It2 DoD (instrumentation FR-19 + mode headless + OpenMP stable).
- Daily sync (15 min) : état méthode 1 / méthode 2 / scripts / mesures.
- Point d’intégration intermédiaire (24/02) : méthode 1 + scripts + smoke p=2 mergés pour sécuriser le chemin critique.
- Buffer explicite (25–26/02) : corrections bloquantes + revalidation QA.
- Gel (27/02) : uniquement fixes critiques.

## Risques & mitigations
- MPI indisponible sur l’environnement cible -> Mitigation : fallback multi-process local (documenter l’écart) + prioriser méthode 1.
- Coût de communication trop élevé (fusion max/halos) -> Mitigation : réduire fréquence d’échange si compatible et mesurer ; documenter limites.
- Déséquilibre de charge (méthode 2, nid local) -> Mitigation : tester ε élevé + instrumenter distribution des fourmis ; accepter speedup limité si justifié.
- Deadlocks/bugs bords (méthode 2) -> Mitigation : tests ciblés halos + smoke p=2, scénarios petits.
- Retard -> Mitigation : prioriser méthode 1 + tableau speedup ; si nécessaire, limiter méthode 2 à p=2 (décomposition 1D + halos + migration) et documenter l’écart.

## Definition of Done (Itération 3)
- [ ] Méthode 1 (FR-22) fonctionne sur p=2 et produit une sortie + CSV.
- [ ] Méthode 2 (FR-23) fonctionne sur p=2 (au minimum) et passe un smoke sans deadlock.
- [ ] Scripts multi-processus (NFR-04) disponibles et documentés.
- [ ] Tableaux d’accélération processus (NFR-03) publiés (au minimum p=1 vs p=2, méthode 1).
- [ ] Tests de régression/invariants (NFR-05/NFR-06) verts (`make test` + smoke distribué).
- [ ] Documentation finale (NFR-07/NFR-08) à jour et reproductible.
- [ ] Gel le 27/02 : aucun bug bloquant ouvert, livrables prêts à rendre.
