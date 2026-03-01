# Extender `scripts/measure_temps.sh` para medir AoS vs SoA (mismo profiling)

## Resumen
La idea es **agregar un flag `--layout aos|soa` al script**, pasarlo al ejecutable (`ant_simu.exe`) y generar **exactamente los mismos artefactos** (`*.metrics`, `raw_measurements.csv`, `table_a.csv`, `table_b.csv`, `amdahl_prediction.csv`, `summary.md`, etc.), pero guardados en **subcarpetas separadas por layout** para no mezclar resultados.

## Cambios de interfaz (CLI + outputs)
### Nueva opción del script
- `./scripts/measure_temps.sh --layout aos`
- `./scripts/measure_temps.sh --layout soa`
- Soportar también `--layout=aos|soa`.
- Default: `aos`.

### Organización de resultados (sin confusión)
Cambiar el output a:
- `results/<specific_folder>/<layout>/<timestamp>/`
- Symlinks:
  - `results/<specific_folder>/<layout>/latest` → última corrida de ese layout
  - (Opcional para compatibilidad) `results/<specific_folder>/latest` → última corrida global (sea AoS o SoA)

## Prerrequisito (importante)
El binario `src/ant_simu.exe` debe aceptar `--layout aos|soa` y **mantener el mismo formato `METRIC ...`**.  
(Si todavía no existe `--layout` en el C++, el script fallará con “Unknown argument: --layout”.)

## Implementación (paso a paso, decisión completa)

### Paso 1 — Parsear `--layout`
Archivo: `scripts/measure_temps.sh`
1. Agregar variables:
   - `LAYOUT="aos"`
   - `LAYOUT_SET=0`
2. En el `case` del parser:
   - `--layout <val>` (con `shift` y validación)
   - `--layout=<val>`
3. Validar `val` ∈ `{aos, soa}`; si no, imprimir error + `print_help` + `exit 1`.

### Paso 2 — Ajustar `OUT_DIR` y symlinks
Archivo: `scripts/measure_temps.sh`
1. Cambiar:
   - `OUT_DIR="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${RUN_ID}"`
   por:
   - `OUT_DIR="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${LAYOUT}/${RUN_ID}"`
2. Crear directorios:
   - `mkdir -p "${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${LAYOUT}"`
3. Symlinks:
   - `LATEST_LINK_LAYOUT="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${LAYOUT}/latest"`
   - (Opcional) mantener `LATEST_LINK_GLOBAL="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/latest"`
4. Al final:
   - `ln -sfn "${OUT_DIR}" "${LATEST_LINK_LAYOUT}"`
   - (Opcional) `ln -sfn "${OUT_DIR}" "${LATEST_LINK_GLOBAL}"`

### Paso 3 — Registrar layout en metadatos
Archivo: `scripts/measure_temps.sh`
1. En `run_config.env` agregar:
   - `layout=${LAYOUT}`
2. En `summary.md` agregar una línea:
   - `- Layout: ${LAYOUT}`

### Paso 4 — Ejecutar el benchmark con el layout elegido
Archivo: `scripts/measure_temps.sh`
Modificar el comando del benchmark para incluir el layout:
- Antes:
  - `"${REPO_ROOT}/src/ant_simu.exe" --benchmark --iterations ... --warmup ...`
- Después:
  - `"${REPO_ROOT}/src/ant_simu.exe" --benchmark --layout "${LAYOUT}" --iterations ... --warmup ...`

### Paso 5 — Actualizar `print_help()` del script
Archivo: `scripts/measure_temps.sh`
- Documentar `--layout aos|soa`
- Actualizar “Outputs” para reflejar la subcarpeta por layout y el symlink `latest` por layout.

### Paso 6 — Actualizar la documentación
Archivo: `docs/instructions/measure_temps.md`
- Agregar la opción `--layout`.
- Cambiar rutas esperadas a:
  - `results/test_time_measurements/<layout>/<timestamp>/`
  - `results/test_time_measurements/<layout>/latest`
  - (Opcional) `results/test_time_measurements/latest`

## Casos de prueba / aceptación
1. `./scripts/measure_temps.sh --layout aos`:
   - crea `results/test_time_measurements/aos/<timestamp>/`
   - genera todos los CSV y `summary.md` como antes
   - crea/actualiza `results/test_time_measurements/aos/latest`
2. `./scripts/measure_temps.sh --layout soa`:
   - crea `results/test_time_measurements/soa/<timestamp>/`
   - mismos artefactos y formato
   - `results/test_time_measurements/soa/latest` actualizado
3. Verificar que el script sigue funcionando con `--folder` y con el argumento posicional del folder:
   - `./scripts/measure_temps.sh --folder my_folder --layout soa`
   - `./scripts/measure_temps.sh my_folder --layout aos`

## Supuestos
- `src/ant_simu.exe` implementa `--layout` y mantiene los mismos keys `METRIC` usados por el script.
- Querés medir AoS y SoA en corridas separadas (no “both” en una sola ejecución).
- Preferís separar outputs por layout (subcarpeta) para evitar mezclar resultados.
