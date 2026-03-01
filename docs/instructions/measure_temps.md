# Instructions d'exécution - Mesure des temps (points 1 à 4)

Ce document décrit comment exécuter la mesure des temps de l'Exercice 4 à l'aide de `scripts/measure_temps.sh`.

## 1) Ce que fait le script

`scripts/measure_temps.sh` exécute automatiquement :

1. Compilation en mode release (`make clean && make all`).
2. 5 répétitions du benchmark avec :
   - 1200 itérations totales.
   - 200 itérations de warm-up écartées.
   - 1000 itérations mesurées.
   - `OMP_NUM_THREADS=1`.
3. Consolidation des résultats pour les points 1 à 4 de `docs/task_temps_mesure.md` :
   - Tableau A.
   - Tableau B.
   - Fraction parallélisable `F`.
   - Prédiction d'Amdahl pour `{2,4,8,16}`.
4. Séparation des sorties par layout (`aos` ou `soa`) dans des sous-dossiers dédiés.

## 2) Exécution

Depuis la racine du projet :

```bash
chmod +x scripts/measure_temps.sh
./scripts/measure_temps.sh
```

Optionnel : changer le nom du dossier spécifique dans `results/` :

```bash
./scripts/measure_temps.sh mon_dossier
./scripts/measure_temps.sh --folder mon_dossier
```

Choisir le layout à mesurer :

```bash
./scripts/measure_temps.sh --layout aos
./scripts/measure_temps.sh --layout soa
./scripts/measure_temps.sh --folder mon_dossier --layout soa
./scripts/measure_temps.sh mon_dossier --layout=aos
```

Afficher l'aide :

```bash
./scripts/measure_temps.sh --help
```

## 3) Emplacement des résultats

Par défaut, les résultats sont enregistrés dans :

```text
results/test_time_measurements/<layout>/<timestamp>/
```

De plus, le lien est mis à jour :

```text
results/test_time_measurements/<layout>/latest
results/test_time_measurements/latest
```

## 4) Fichiers générés

Dans le dossier de l'exécution (`<timestamp>/`), les fichiers suivants sont générés :

- `run_config.env`: configuration de l'exécution.
- `system_info.txt`: informations matériel/OS.
- `build.log`: journal de compilation.
- `rep_*.metrics`: métriques brutes par répétition.
- `rep_*.stderr`: sortie d'erreur standard par répétition.
- `raw_measurements.csv`: consolidation brute par répétition.
- `init_times.csv`: résumé one-shot de `P0`, `P1`, `P2`.
- `table_a.csv`: résumé du tableau A.
- `table_b.csv`: résumé du tableau B.
- `amdahl_prediction.csv`: speedup théorique selon Amdahl.
- `summary.md`: résumé final au format Markdown.

## 5) Sortie attendue dans la console

Le script affiche l'avancement en 4 étapes :

1. Compilation en mode release.
2. Exécution des répétitions.
3. Génération du résumé.
4. Chemin final des résultats.
