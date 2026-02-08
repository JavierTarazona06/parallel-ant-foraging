# Plan d’itérations (08/02/2026 → 27/02/2026)

**Équipe :** Javier, Daniel

## Hypothèses
- SDL2 et toolchain C++17 disponibles sur l’environnement cible ; OpenMP compilable.
- Exécution distribuée : utilisation de processus locaux (MPI ou équivalent) possible ; sinon prototypage limité au pseudo-MPI (assumption, à invalider/adapter).
- Une seule machine de test principale ; mesures répétables avec mêmes graines et mêmes paramètres.
- Pas de contrainte UI avancée ; rendu SDL suffit pour validation fonctionnelle.
- Accès à un outil de profilage basique (std::chrono + scripts) pour mesures.

## Vue d’ensemble
| Itération | Dates | Objectif | Livrable |
|---|---|---|---|
| Itération 0 | 08–09/02 | Cadrage, backlog affiné, outillage temps/tests | Plan validé, harness de mesures squelette |
| Itération 1 | 10–14/02 | Couverture exigences fonctionnelles cœur (FR-01→15,18) + tests de base | Simulation séquentielle stable, tests unitaires de phéromones/déplacement |
| Itération 2 | 15–21/02 | Performance : vectorisation, OpenMP, instrumentation, premières mesures (FR-19→21, NFR-01→03) | Version vectorisée + OpenMP, tableau d’accélération threads, rapport mesures v1 |
| Itération 3 | 22–27/02 | Parallélisation distribuée, stabilisation finale, doc livraison (FR-22, FR-23, NFR-04→08) | Prototype distribué (2 méthodes), tableaux d’accélération processus, doc finale, buffer QA |

Chemin critique : (1) stabiliser cœur simulation (It1) → (2) instrumentation + vectorisation/OpenMP (It2) → (3) distribué (It3). Toute dérive sur (1) retarde (2) et (3).

## Itération 0 — Démarrage & cadrage (08–09/02)
- Objectifs  
  - Installer/valider toolchain C++17 + SDL2 + OpenMP.  
  - Affiner backlog, clarifier hypothèses restantes.  
  - Esquisser harness de mesure (chrono + scripts) sans modifier logique.
- Exigences ciblées (FR/NFR) : NFR-04 (reproductibilité, graines), NFR-07 (maintenabilité), préparation NFR-01.
- Tâches (checklist)  
  - Vérifier build existant, ajouter cible `debug`/`release` dans Makefile si besoin.  
  - Documenter commandes de build/run et graines par défaut.  
  - Esquisser script de mesure (pseudo-code) et structure de sortie (CSV).  
  - Backlog détaillé par exigence avec effort estimé.
- Répartition  
  - Javier : build/toolchain, backlog, hypothèses.  
  - Daniel : script de mesure squelette, doc run.
- Dépendances : aucune.
- Critères d’acceptation  
  - Build reproductible en local (debug + release).  
  - Graines et paramètres d’exécution documentés.  
  - Script/harness de mesure prêt à instrumenter (même si stubs).  
  - Backlog par FR/NFR validé par l’équipe.
- Risques + mitigation  
  - Risque : dépendances SDL manquantes → Mitigation : fallback paquet précompilé, doc installation.  
  - Risque : outillage chrono insuffisant → Mitigation : prévoir alternative simple (time command + logs).

## Itération 1 — Simulation séquentielle stabilisée (10–14/02)
- Objectifs  
  - Couvrir intégralement logique cœur ACO séquentielle et corrections de robustesse.  
  - Tests unitaires/integ de phéromones et déplacement.
- Exigences ciblées : FR-01 à FR-15, FR-18 ; NFR-04, NFR-05, NFR-06 (partie robustesse de base), NFR-07 (structure code).
- Tâches  
  - Valider/compléter représentation grille/types (FR-01/02/13).  
  - Sécuriser budget de mouvement vs coût 0 et sorties de grille (FR-14, robustesse).  
  - Tests unitaires : mise à jour V1/V2 (FR-09/10/11), évaporation (FR-15).  
  - Test d’intégration : compteur nourriture et transitions d’état (FR-05→08).  
  - Vérifier normalisation plasma (FR-18) avec test de plage [0,1].  
  - Ajuster doc d’usage (paramètres m, α, β, ε).
- Répartition  
  - Daniel : implémentations correctives + tests unitaires/intégration.  
  - Javier : revue code, alignement spécification, support CI locale.
- Dépendances : It0 (build/outillage).
- Critères d’acceptation  
  - Tous tests unitaires/intégration verts.  
  - Aucune fuite hors-grille, boucle infinie, ni déplacement sur cellule indésirable.  
  - Simulation séquentielle produit compteur >0 sur scénario simple.  
  - Doc d’exécution mise à jour (paramètres + graines).
- Risques + mitigation  
  - Risque : temps sur corrections inattendues → Mitigation : prioriser FR-05→15, laisser UI pour plus tard.  
  - Risque : manque de tests SDL → Mitigation : tester logique hors rendu.

## Itération 2 — Vectorisation, OpenMP, mesures (15–21/02)
- Objectifs  
  - Structurer données pour vectorisation (SOA) et paralléliser boucles adaptées via OpenMP.  
  - Instrumenter temps par module et produire premiers tableaux d’accélération (threads).
- Exigences ciblées : FR-19, FR-20, FR-21 ; NFR-01, NFR-02, NFR-05 (cohérence après parallélisme), NFR-06 (stress perf).
- Tâches  
  - Refactor données fourmis en tableaux (positions, états, graines) et adapter logique (FR-20).  
  - Identifier boucles parallélisables (MAJ phéromones, déplacement, évaporation) et ajouter pragmas OpenMP (FR-21).  
  - Instrumentation fine : temps total/itération + temps par module (FR-19, NFR-01).  
  - Campagne de mesures : 1, 2, 4, 8 threads ; produire tableau d’accélération (NFR-02).  
  - Vérifications de cohérence phéromones post-parallélisation (NFR-05).
- Répartition  
  - Javier : architecture SOA, parallélisation OpenMP, revue perf.  
  - Daniel : instrumentation chrono/CSV, scripts de bench, exécution et consolidation résultats.
- Dépendances : It1 (simulation stable).
- Critères d’acceptation  
  - Version vectorisée compilée et validée par tests existants.  
  - Tableau d’accélération threads disponible (au moins 3 points).  
  - Aucune régression fonctionnelle observée.  
  - Rapport de mesures (version, params, machine, graines).
- Risques + mitigation  
  - Risque : contention mémoire ↔ OpenMP → Mitigation : planifier essais de scheduling/chunk.  
  - Risque : mesures bruitées → Mitigation : répéter runs, fixer affinité/CPU si possible.

## Itération 3 — Distribué, stabilisation & livraison (22–27/02)
- Objectifs  
  - Implémenter deux stratégies distribuées (méthode 1 : carte complète, fusion max ; méthode 2 : découpage spatial).  
  - Finaliser doc, tests de non-régression, buffer de stabilisation avant 27/02.
- Exigences ciblées : FR-22, FR-23 ; NFR-03, NFR-04, NFR-05, NFR-06, NFR-07, NFR-08.
- Tâches  
  - Méthode 1 : distribution des fourmis, fusion phéromones par max, mesures accélération processus (FR-22, NFR-03).  
  - Méthode 2 : partition spatiale + échanges de bords/halos, migration fourmis, mesures (FR-23, NFR-03).  
  - Scripts d’exécution multi-processus (paramètres reproductibles, NFR-04).  
  - Campagnes de mesure processus (1,2,4,…), tableaux d’accélération.  
  - Documentation finale : how-to run, paramètres, limites connues, résultats.  
  - Stabilisation/régression : re-lancer tests It1 + perf It2, fixer bugs critiques.  
  - Buffer 2 jours (25–26/02) pour fix/QA, 27/02 gel.
- Répartition  
  - Daniel : implémentation distribué (méthodes 1 & 2), scripts multi-proc, mesures.  
  - Javier : revue architecture, intégration finale, QA, packaging livrables.
- Dépendances : It2 (instrumentation + version stable).
- Critères d’acceptation  
  - Deux stratégies distribuées fonctionnelles sur au moins 2 processus.  
  - Tableaux d’accélération processus publiés.  
  - Tests de régression verts ; doc finale disponible.  
  - Jour 27/02 : gel, aucun bug bloquant ouvert.
- Risques + mitigation  
  - Risque : environnement MPI indisponible → Mitigation : fallback multi-processus local + pseudo-fusion fichiers ; documenter écart.  
  - Risque : déséquilibre charge méthode 2 → Mitigation : tests avec ε élevé et redistribution simple si temps.  
  - Risque : retard → Mitigation : buffer 2 jours, scope réduit (prioriser méthode 1).

## Tableau de traçabilité (itérations ↔ exigences)
| Exigence | Itération(s) | Tâche(s) clé | Preuve attendue (test/mesure/doc) |
|---|---|---|---|
| FR-01 | It1 | Représentation grille/types | Tests d’intégration déplacement/grille |
| FR-02 | It1 | Config nid/nourriture | Test init multi-sources (si supporté) |
| FR-03 | It1 | Carte phéromones | Tests unitaires stockage V1/V2 |
| FR-04 | It1 | Init fourmis | Test init positions/états |
| FR-05 | It1 | Compteur nourriture | Test intégration compteur |
| FR-06 | It1 | État chargé sur nourriture | Test transition |
| FR-07 | It1 | État non chargé sur nid | Test transition |
| FR-08 | It1 | Incrément compteur | Test compteur |
| FR-09 | It1 | Max/moyenne voisins | Test MAJ locale phéromones |
| FR-10 | It1 | MAJ V1 (α, cas nourriture) | Test formule |
| FR-11 | It1 | MAJ V2 (α, cas nid) | Test formule |
| FR-12 | It1 | Déplacement ε / 1−ε | Test probas / argmax |
| FR-13 | It1 | Interdiction indésirables | Test obstacles/bords |
| FR-14 | It1 | Budget mouvement | Test coût cellule + boucle bornée |
| FR-15 | It1 | Évaporation β | Test décroissance V1/V2 |
| FR-16 | It1 | Plasma structure | Test dimensions n_s, bords |
| FR-17 | It1 | Déviation plasma | Test bornes générateur |
| FR-18 | It1 | Normalisation [0,1] | Test min/max carte |
| FR-19 | It2 | Mesures de temps | Script chrono + CSV |
| FR-20 | It2 | Vectorisation SOA | Bench + tests régression |
| FR-21 | It2 | OpenMP + accélération threads | Tableau accélération threads |
| FR-22 | It3 | Distribué méthode 1 | Tableau accélération processus + logs fusion max |
| FR-23 | It3 | Distribué méthode 2 | Tableau accélération processus + logs échanges bords |
| NFR-01 | It2 | Mesures comparatives | Rapport mesures |
| NFR-02 | It2 | Scalabilité threads | Tableau accélération threads |
| NFR-03 | It3 | Scalabilité processus | Tableau accélération processus |
| NFR-04 | It0→It3 | Graines fixes | Doc paramètres + scripts |
| NFR-05 | It1→It3 | Cohérence phéromones/bords | Tests cohérence + revues parallèles |
| NFR-06 | It1→It3 | Robustesse tailles/ε | Tests stress, valeurs extrêmes |
| NFR-07 | It0→It3 | Maintenabilité/modularité | Structure modules + doc archi |
| NFR-08 | It3 | Portabilité | Build/test sur env cible documenté |

## Critères de livraison finale (27/02/2026)
- Code compilable (release et debug), tests unitaires/intégration verts.
- Versions livrées : séquentielle, vectorisée, OpenMP, distribuée (2 méthodes) avec scripts d’exécution.
- Tableaux d’accélération : threads (It2) et processus (It3), paramètres/plateforme documentés.
- Documentation : mode d’emploi, paramètres (m, α, β, ε, graines), limites connues, protocole de mesure.
- Artifacts : binaire(s), scripts de bench, logs/CSV de mesures, rapport de performance.
- Buffer de stabilisation respecté, aucun blocant ouvert.
