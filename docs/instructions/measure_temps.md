# Instrucciones de ejecución - Medición de tiempos (puntos 1 a 4)

Este documento describe cómo ejecutar la medición de tiempos del Ejercicio 4 usando `scripts/measure_temps.sh`.

## 1) Qué hace el script

`scripts/measure_temps.sh` ejecuta automáticamente:

1. Build release (`make clean && make all`).
2. 5 repeticiones del benchmark con:
   - 1200 iteraciones totales.
   - 200 iteraciones de warm-up descartadas.
   - 1000 iteraciones medidas.
   - `OMP_NUM_THREADS=1`.
3. Consolidación de resultados para los puntos 1 a 4 de `docs/task_temps_mesure.md`:
   - Tabla A.
   - Tabla B.
   - Fracción paralelizable `F`.
   - Predicción de Amdahl para `{2,4,8,16}`.

## 2) Ejecución

Desde la raíz del proyecto:

```bash
chmod +x scripts/measure_temps.sh
./scripts/measure_temps.sh
```

Opcional: cambiar el nombre de carpeta específica dentro de `results/`:

```bash
./scripts/measure_temps.sh mi_carpeta
./scripts/measure_temps.sh --folder mi_carpeta
```

Mostrar ayuda:

```bash
./scripts/measure_temps.sh --help
```

## 3) Ubicación de resultados

Por defecto, los resultados se guardan en:

```text
results/test_time_measurements/<timestamp>/
```

Además se actualiza el enlace:

```text
results/test_time_measurements/latest
```

## 4) Archivos generados

En la carpeta de la corrida (`<timestamp>/`) se generan:

- `run_config.env`: configuración de la corrida.
- `system_info.txt`: datos de hardware/SO.
- `build.log`: log de compilación.
- `rep_*.metrics`: métricas crudas por repetición.
- `rep_*.stderr`: salida de error por repetición.
- `raw_measurements.csv`: consolidado crudo por repetición.
- `init_times.csv`: resumen one-shot de `P0`, `P1`, `P2`.
- `table_a.csv`: resumen Tabla A.
- `table_b.csv`: resumen Tabla B.
- `amdahl_prediction.csv`: speedup teórico por Amdahl.
- `summary.md`: resumen final en formato Markdown.

## 5) Salida esperada en consola

El script imprime progreso en 4 pasos:

1. Build release.
2. Ejecución de repeticiones.
3. Generación de resumen.
4. Ruta final de resultados.
