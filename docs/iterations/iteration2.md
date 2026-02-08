# Itération 2 — Vectorisation, OpenMP, mesures (15–21/02)

## Objectifs & livrables
- Instrumentation des temps par itération et par module (generation environnement, mise a jour pheromones, deplacement, evaporation, rendu si active).
- Version vectorisee (layout en tableaux / SOA) ou les fourmis sont reperees par un indice dans les tableaux.
- Version OpenMP (memoire partagee) sur les boucles pertinentes, sans regression fonctionnelle visible.
- Campagne de mesures threads et tableau d'acceleration (speedup) en fonction du nombre de threads et du nombre de coeurs de la machine.
- Rapport de mesures v1 (CSV + tableau Markdown) permettant la comparaison : reference vs vectorisee vs OpenMP.

## Exigences ciblees (FR / NFR)
- FR-19, FR-20, FR-21
- NFR-01, NFR-02, NFR-05, NFR-06

## Plan de travail — Javier
| ID tache | Titre | Priorite | Estimation (h) | Dependances | Livrable/Preuve |
|---|---|---|---:|---|---|
| J2-01 | Design layout SOA + API de simulation | P0 | 6 | It1 DoD | `src/ants_soa.*` + `src/sim_step.*` (ou equivalent) |
| J2-02 | Implementer la version vectorisee (SOA) | P0 | 10 | J2-01, D2-01 (API timings) | Binaire/flag `--mode=soa` ou equivalent + tests OK |
| J2-03 | Activer OpenMP + strategie de concurrence pheromones | P0 | 10 | J2-02 | Version OpenMP stable + gain mesurable sur au moins 1 scenario |
| J2-04 | Tuning OpenMP (scheduling/memoire) | P1 | 6 | J2-03, D2-02 | Parametres OpenMP documentes + mesures comparees |
| J2-05 | Integration (merge) + verification bout-en-bout | P0 | 4 | J2-02..J2-04, D2-01..D2-04 | Merge + `make test` + bench baseline + tableau speedup |

[Tache J2-01] — Design layout SOA + API de simulation

Priorite : P0

Estimation : 6 h

Exigences liees : FR-20 ; NFR-07

Description :
Definir une representation en tableaux (SOA) pour les fourmis et une API claire pour executer une iteration de simulation. Objectif : permettre une version vectorisee sans casser la version de reference et sans melanger rendu/SDL avec la logique.

Etapes :
- [ ] Identifier les donnees minimales par fourmi : position (x,y), etat (loaded/unloaded), graine RNG.
- [ ] Definir une structure `AntsSOA` (ou equivalent) avec des `std::vector<int>`/`std::vector<uint32_t>` contigus.
- [ ] Definir une fonction d'etape de simulation "headless" (sans rendu) : `step(land, pheromones, ants, ...)`.
- [ ] Definir une interface de configuration scenario (m, alpha, beta, eps, graines, positions nid/nourriture) sans I/O lourde.
- [ ] Documenter l'API (comment appeler, invariants, ownership).

Fichiers / modules cibles :
- Nouveau : `src/ants_soa.hpp`, `src/ants_soa.cpp` (ou `src/ants_soa.hpp` uniquement)
- Nouveau : `src/sim_step.hpp`, `src/sim_step.cpp` (extraction de la logique de `advance_time`)
- `src/ant_simu.cpp` (orchestration / selection mode)

Dependances :
- It1 : simulation reference stable + tests verts.

Criteres d'acceptation :
- Une iteration de simulation peut etre executee sans ouvrir de fenetre SDL.
- Le code separe clairement : logique simulation vs rendu vs mesures.

Livrable / Preuve :
- Compilation OK + note d'architecture courte (dans un commentaire de PR ou `docs/iterations/iteration2.md` si besoin).

[Tache J2-02] — Implementer la version vectorisee (SOA)

Priorite : P0

Estimation : 10 h

Exigences liees : FR-20 ; NFR-05, NFR-06

Description :
Produire une version ou les fourmis sont representees par tableaux (positions, etats, graines). La logique doit rester equivalente a la reference au niveau des invariants (pas de sortie de grille, pas sur indésirable, compteur cohérent).

Etapes :
- [ ] Implementer `advance_soa()` ou equivalent : boucle sur indice de fourmi, lecture/ecriture positions/etats.
- [ ] Remplacer l'utilisation de `ant` objets pour le mode SOA (sans supprimer le mode reference).
- [ ] Garantir la reproductibilite : un indice de fourmi -> une graine stable, meme scenario -> memes sorties (dans la mesure du possible).
- [ ] Ajouter un test de non-regression : reference vs SOA sur scenario deterministe (meme graine) (coordination avec D2-03).
- [ ] S'assurer que la boucle de mouvement reste bornee (heritage It1) dans le mode SOA.

Fichiers / modules cibles :
- `src/ants_soa.hpp`, `src/ants_soa.cpp`
- `src/sim_step.hpp`, `src/sim_step.cpp`
- `src/pheronome.hpp`
- `src/ant_simu.cpp` (switch mode, scenario)
- `tests/*` (non-regression, si existants)

Dependances :
- J2-01.
- D2-01 (pour avoir des hooks de timings stables sur les modules).

Criteres d'acceptation :
- Mode SOA compile et s'execute en headless.
- Les tests existants passent, plus un test de comparaison reference vs SOA (au moins sur compteur nourriture et invariants).

Livrable / Preuve :
- Log `make test` + sortie headless (ex: 100 iterations) + capture du CSV de timings sur mode SOA.

[Tache J2-03] — Activer OpenMP + strategie de concurrence pheromones

Priorite : P0

Estimation : 10 h

Exigences liees : FR-21 ; NFR-01, NFR-02, NFR-05

Description :
Parallelliser en memoire partagee avec OpenMP les boucles qui apportent un gain net (evaporation, rendu optionnel, et potentiellement boucle fourmis). Le point critique est la coherence des pheromones lors d'ecritures concurrentes : definir une strategie explicite et testable.

Etapes :
- [ ] Ajouter le support compilation OpenMP (flags + include) et un parametre `threads` (env OMP_NUM_THREADS ou flag).
- [ ] Parallelliser `do_evaporation()` (boucles i/j sur la grille) avec `#pragma omp parallel for collapse(2)`.
- [ ] Choisir la strategie pour la mise a jour des pheromones : Option A (recommandee) buffers locaux par thread + fusion par maximum sur la carte buffer.
- [ ] Prevoir un fallback : Option B sections critiques/atomiques (a eviter ; n'utiliser que si justifie par une preuve).
- [ ] Implementer la strategie retenue et ajouter un test de coherence (D2-03).
- [ ] Mesurer T(1) vs T(p) et verifier que `S(p)=T(1)/T(p)` est calculable (D2-02).

Fichiers / modules cibles :
- `src/Makefile` (flags OpenMP)
- `src/pheronome.hpp` (evaporation + strategie merge)
- `src/sim_step.*` (boucles paralleles)
- `src/ant_simu.cpp` (param threads, mode headless)
- `tests/*` (coherence)

Dependances :
- J2-02 (mode SOA fonctionnel).

Criteres d'acceptation :
- Execution OpenMP (p=2) donne le meme resultat "qualitatif" que p=1 sur scenario deterministe (a minima invariants + compteur comparable).
- Gain mesurable sur au moins un scenario representatif (rapport D2-02).

Livrable / Preuve :
- Log de run avec `OMP_NUM_THREADS=1/2/4` + extrait CSV montrant T(p) et S(p).

[Tache J2-04] — Tuning OpenMP (scheduling/memoire)

Priorite : P1

Estimation : 6 h

Exigences liees : NFR-01, NFR-02

Description :
Ameliorer la performance en ajustant la granularite (collapse, chunk), le scheduling et en reduisant le faux partage/memoire inutile, sans changer le perimetre fonctionnel.

Etapes :
- [ ] Evaluer `static` vs `dynamic` scheduling sur les boucles cibles.
- [ ] Ajuster `chunk` si pertinent et documenter les choix.
- [ ] Verifier l'alignement/contiguite des tableaux SOA (reserve, shrink_to_fit interdit).
- [ ] Produire une comparaison "avant/apres tuning" sur 1 scenario fixe.

Fichiers / modules cibles :
- `src/sim_step.*`
- `src/ants_soa.*`
- `docs/iterations/iteration2.md` (notes de tuning, si besoin)

Dependances :
- J2-03.
- D2-02 (harness de bench stable).

Criteres d'acceptation :
- Le tuning ne change pas la sortie fonctionnelle observable.
- Un tableau de comparaison montre une amelioration ou une justification (pas de regression majeure).

Livrable / Preuve :
- CSV + note courte des parametres OpenMP testes.

[Tache J2-05] — Integration (merge) + verification bout-en-bout

Priorite : P0

Estimation : 4 h

Exigences liees : FR-19, FR-20, FR-21 ; NFR-01, NFR-02, NFR-05, NFR-06

Description :
Integrer les changements It2 sur la branche principale et verifier build, tests et bench. Produire le tableau d'acceleration threads (livrable It2).

Etapes :
- [ ] Feature freeze It2 (21/02 midi).
- [ ] Merger les PR It2 (instrumentation, SOA, OpenMP) dans un ordre reduisant les conflits.
- [ ] Lancer `make clean && make all` (debug/release si disponibles).
- [ ] Lancer `make test` et archiver la sortie.
- [ ] Lancer la campagne de bench definie (D2-02) et generer le tableau speedup threads.
- [ ] Verifier que les artefacts (CSV + tableau) sont versionnes dans `docs/` ou attaches au merge.

Fichiers / modules cibles :
- Branche principale (integration)
- `src/Makefile`, `src/*` modifies
- `tests/*`
- `docs/` (resultats, tableau)

Dependances :
- J2-02..J2-04 terminees.
- D2-01..D2-04 terminees (ou en etat "pret pour merge").

Criteres d'acceptation :
- Build + tests OK.
- Tableau speedup threads present (au moins 1,2,4,8 threads si disponibles) avec protocole documente.

Livrable / Preuve :
- Commit(s) de merge + logs de commandes + tableau Markdown dans `docs/`.

## Plan de travail — Daniel
| ID tache | Titre | Priorite | Estimation (h) | Dependances | Livrable/Preuve |
|---|---|---|---:|---|---|
| D2-01 | Instrumentation temps par module + export CSV | P0 | 8 | It1 DoD | CSV par run + doc protocole |
| D2-02 | Harness bench threads + calcul tableau speedup | P0 | 8 | D2-01 | Tableau acceleration threads + scripts |
| D2-03 | Validation non-regression (reference vs SOA vs OpenMP) | P0 | 6 | D2-01, J2-02, J2-03 | Tests verts + criteres NFR-05 |
| D2-04 | Rapport mesures v1 (Markdown) + archivage artefacts | P1 | 4 | D2-02, D2-03 | `docs/` mis a jour (tableaux + explications) |

[Tache D2-01] — Instrumentation temps par module + export CSV

Priorite : P0

Estimation : 8 h

Exigences liees : FR-19 ; NFR-01

Description :
Mesurer et consigner le temps passe par iteration et par grandes parties du code. Objectif : permettre des comparaisons fiables entre versions (reference, SOA, OpenMP) sur un protocole identique.

Etapes :
- [ ] Definir la liste des modules mesures (minimum) : total iteration, boucle fourmis, mise a jour pheromones, evaporation, generation/normalisation (si incluse), rendu (si active).
- [ ] Implementer une API de timing (RAII ou macros) basee sur `std::chrono` produisant des mesures cumulatives.
- [ ] Exporter un CSV par run avec colonnes : mode, threads, m, alpha, beta, eps, seed_land, seed_ants, nb_iter, t_total, t_ants, t_pher, t_evap, etc.
- [ ] Ajouter une option headless pour eviter SDL pendant bench (si pas deja fait).
- [ ] Valider sur 2 runs consecutifs que le format CSV est stable.

Fichiers / modules cibles :
- Nouveau : `src/timing.hpp`, `src/timing.cpp` (ou header-only)
- `src/sim_step.*` (points de mesure)
- `src/ant_simu.cpp` (parametrage, sortie CSV)
- `docs/` (protocole de mesure court)

Dependances :
- It1 DoD (simulation stable + tests).

Criteres d'acceptation :
- Un run produit un CSV exploitable sans post-traitement manuel.
- Les mesures sont associees a un scenario complet (parametres + graines).

Livrable / Preuve :
- Exemple de CSV (commit) + commande de run documentee.

[Tache D2-02] — Harness bench threads + calcul tableau speedup

Priorite : P0

Estimation : 8 h

Exigences liees : FR-21 ; NFR-01, NFR-02

Description :
Automatiser une campagne de mesures sur plusieurs nombres de threads et produire un tableau d'acceleration `S(p)=T(1)/T(p)`.

Etapes :
- [ ] Definir un scenario standard (taille carte, m, alpha, beta, eps, graines, nb_iter) et le geler.
- [ ] Creer un script (shell) qui lance les runs pour p=1,2,4,8 (et autres si disponibles) et collecte les CSV.
- [ ] Implementer un petit outil de post-traitement (awk/sed ou petit programme C++ si prefere) pour calculer T(p) et S(p).
- [ ] Produire un tableau Markdown "threads vs temps vs speedup" et l'archiver dans `docs/`.
- [ ] Repeter 3 fois et reporter moyenne/mediane (si temps) ou au minimum 2 runs pour limiter le bruit.

Fichiers / modules cibles :
- Nouveau : `scripts/bench_threads.sh` (ou `tools/bench_threads.sh`)
- Nouveau : `scripts/summarize_speedup.sh` ou `tools/summarize_speedup.cpp`
- `docs/` (tableau resultats)

Dependances :
- D2-01 (CSV stable).
- J2-03 (OpenMP fonctionnel) pour la partie OpenMP du tableau.

Criteres d'acceptation :
- Tableau speedup threads produit et reproductible.
- Le protocole (scenario + commandes) est documente.

Livrable / Preuve :
- Script(s) + tableau Markdown + artefacts CSV.

[Tache D2-03] — Validation non-regression (reference vs SOA vs OpenMP)

Priorite : P0

Estimation : 6 h

Exigences liees : NFR-05, NFR-06 ; FR-19, FR-20, FR-21 (validation)

Description :
Assurer qu'aucune optimisation (SOA/OpenMP) ne casse la correction du modele. Les resultats peuvent differer legerement en parallele, mais les invariants et criteres doivent rester satisfaits.

Etapes :
- [ ] Definir des invariants testables : pas de sortie de grille, pas sur indésirable, compteur >= 0, pheromones dans [0,1] ou bornes attendues, V1=1 sur nourriture, V2=1 sur nid apres update.
- [ ] Ajouter/etendre des tests qui executent un petit nombre d'iterations en mode reference et SOA.
- [ ] Comparer automatiquement : invariants identiques (scenario deterministe).
- [ ] Comparer automatiquement : compteur nourriture identique (scenario deterministe).
- [ ] Ajouter un test OpenMP p=2 qui verifie au moins invariants (et compteur "raisonnable").
- [ ] Documenter les ecarts acceptes (si parallele non deterministe) et pourquoi.

Fichiers / modules cibles :
- `tests/*` (nouveaux tests de comparaison)
- `src/ants_soa.*`, `src/sim_step.*`
- `src/pheronome.hpp`

Dependances :
- D2-01 (headless + scenario fixe).
- J2-02 (SOA).
- J2-03 (OpenMP).

Criteres d'acceptation :
- `make test` passe pour les trois modes (reference, SOA, OpenMP).
- Les invariants sont verifies de facon automatique.

Livrable / Preuve :
- Log `make test` + description des invariants et ecarts acceptes.

[Tache D2-04] — Rapport mesures v1 (Markdown) + archivage artefacts

Priorite : P1

Estimation : 4 h

Exigences liees : FR-19 ; NFR-01, NFR-02

Description :
Consolider les resultats (CSV + tableaux) et produire un rapport lisible pour It2 (comparaison reference/vectorisee/OpenMP).

Etapes :
- [ ] Inserer le tableau speedup threads dans un fichier `docs/perf_iteration2.md` (ou section dans docs existants).
- [ ] Decrire le protocole : machine, nb coeurs, scenario, nb runs, moyenne/mediane.
- [ ] Ajouter une section "observations" : goulots, limites, prochaines actions (It3 distribue).
- [ ] Verifier que les CSV associes sont conserves (dans `docs/` ou en artefacts commit).

Fichiers / modules cibles :
- Nouveau : `docs/perf_iteration2.md` (ou `docs/iterations/iteration2_results.md`)
- `docs/iterations/iteration2.md` (optionnel : lien vers rapport)

Dependances :
- D2-02 et D2-03.

Criteres d'acceptation :
- Rapport contient au moins : tableau threads, temps, speedup, protocole.
- Un lecteur peut reproduire les mesures a partir des commandes documentees.

Livrable / Preuve :
- Rapport Markdown + CSV versionnes.

## Dependances & synchronisation
- Dependances d'iteration : It1 DoD (simulation stable + tests).
- Daily sync (15 min) : etat instrumentation (D2-01) + etat SOA/OpenMP (J2-02/J2-03).
- Point d'integration intermediaire (18/02) : merger instrumentation + headless + premiers CSV pour debloquer benches.
- Fenetre bench (20/02) : gel parametres scenario, execution campagne complete.
- Feature freeze (21/02 midi) puis integration finale (J2-05).

## Risques & mitigations
- Races pheromones en OpenMP (NFR-05) -> Mitigation : strategie buffers thread-local + merge max, tests D2-03.
- Mesures bruitées (NFR-01) -> Mitigation : scenario fixe + repetitions + isoler rendu SDL (headless).
- Refactor SOA casse la logique -> Mitigation : garder mode reference, tests de comparaison D2-03.
- Gain OpenMP insuffisant -> Mitigation : cibler evaporation + sections lourdes, tuning J2-04, documenter limites.

## Definition of Done (Itération 2)
- [ ] Instrumentation FR-19 : CSV par run incluant temps par module et meta (mode, threads, parametres, graines).
- [ ] Vectorisation FR-20 : mode SOA disponible et execute au moins 100 iterations headless sans crash.
- [ ] OpenMP FR-21 : compilation avec OpenMP et execution avec `OMP_NUM_THREADS>1` sans crash.
- [ ] Validation NFR-05/NFR-06 : tests non-regression/invariants passent (`make test`).
- [ ] Mesures NFR-01/NFR-02 : tableau speedup threads produit (au minimum p=1,2,4,8 si disponibles) avec protocole documente.
- [ ] Integration : merge final (J2-05) + logs build/tests/bench archives dans `docs/` ou description de merge.
