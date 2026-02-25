# Ejercicio 4 - Medición del tiempo por partes del código

## 1) Objetivo y alcance

Este documento define **qué medir**, **cómo medir** y **qué presentar** para el ejercicio 4 del proyecto (versión actual del código), con foco en el **kernel de simulación** y una medida **separada** del renderizado SDL.

Alcance de este entregable:

- Versión actual (no vectorizada y no paralelizada con OpenMP en los bucles de negocio).
- Solo documento metodológico.
- Ningún cambio obligatorio en APIs públicas.

## 2) Partes del código a medir

| ID | Parte | Qué medir | Referencia de código |
|---|---|---|---|
| P0 | Inicialización del terreno | Tiempo one-shot de generación fractal | `src/fractal_land.cpp:26` |
| P1 | Normalización del terreno | Tiempo one-shot de min/max + rescale | `src/ant_simu.cpp:37` |
| P2 | Inicialización de hormigas | Tiempo one-shot de creación de la población | `src/ant_simu.cpp:54` |
| K0 | Kernel por iteración (total) | Tiempo de `advance_time` | `src/ant_simu.cpp:11` |
| K1 | Avance de hormigas (total) | Bucle `for` sobre todas las hormigas | `src/ant_simu.cpp:15` |
| K2 | Lógica interna de una hormiga | Decisión + desplazamiento + estado | `src/ant.cpp:14` |
| K3 | Marcado de feromonas | Coste de `mark_pheronome` | `src/ant.cpp:46`, `src/pheronome.hpp:73` |
| K4 | Evaporación | Coste de `do_evaporation` | `src/pheronome.hpp:65` |
| K5 | Swap / actualización del mapa | Coste de `update` | `src/pheronome.hpp:101` |
| R0 | Render por iteración | `renderer.display` + `present` | `src/renderer.cpp:23`, `src/ant_simu.cpp:77` |
| E0 | Gestión de eventos | Bucle `SDL_PollEvent` | `src/ant_simu.cpp:72` |

Notas:

- El render (`R0`) se mide, pero **no es** la métrica principal para el análisis de paralelización.
- Los bloques `P0`, `P1`, `P2` son costes one-shot (no iterativos).

## 3) Cómo medir (protocolo reproducible)

### 3.1 Build y configuración base

```bash
cd src
make clean
make all
```

Baseline exigida:

```bash
OMP_NUM_THREADS=1 ./ant_simu.exe
```

### 3.2 Instrumentación temporal

Usar un reloj monotónico: `std::chrono::steady_clock`.

Principio:

- Iniciar/detener un temporizador alrededor de cada bloque `P/K/R/E`.
- Acumular tiempos (en nanosegundos o microsegundos).
- Convertir a `ms/iter` durante el postprocesado.

Recomendación (implementación futura):

- Instrumentación activable por macro (ejemplo: `-DTIME_PROFILE`) para no contaminar el binario nominal.
- Exportación de medidas en formato CSV.

### 3.3 Protocolo de ejecución

- 5 repeticiones independientes.
- Para cada repetición:
  - 1200 iteraciones totales.
  - 200 iteraciones de warm-up (ignoradas).
  - 1000 iteraciones medidas.
- Mantener constantes: seed, número de hormigas, parámetros (`alpha`, `beta`, `eps`) y build release.

### 3.4 Información de máquina a reportar

Registrar como mínimo:

- CPU, número de núcleos físicos/lógicos.
- SO y kernel.
- Contexto de ejecución (aquí WSL2).

Comandos útiles:

```bash
uname -a
lscpu
nproc
```

## 4) Resultados a presentar (tablas + fórmulas)

### 4.1 Tabla A (base ejercicio 4)

Columnas obligatorias: `Parte`, `ms/iter medio`, `desviación estándar`, `% sobre kernel`.

Plantilla:

| Parte | ms/iter medio | desviación estándar | % sobre kernel |
|---|---:|---:|---:|
| K1 |  |  |  |
| K2 |  |  |  |
| K3 |  |  |  |
| K4 |  |  |  |
| K5 |  |  |  |
| R0 (fuera de kernel) |  |  | n/a |
| E0 (fuera de kernel) |  |  | n/a |

### 4.2 Tabla B (visión global)

Columnas obligatorias: `T_iter_total`, `T_kernel`, `T_render`, `hormigas/s`.

Plantilla:

| Repetición | T_iter_total (ms) | T_kernel (ms) | T_render (ms) | hormigas/s |
|---|---:|---:|---:|---:|
| 1 |  |  |  |  |
| 2 |  |  |  |  |
| 3 |  |  |  |  |
| 4 |  |  |  |  |
| 5 |  |  |  |  |
| Media |  |  |  |  |
| Desviación estándar |  |  |  |  |

### 4.3 Indicadores para análisis de paralelización

Fracción paralelizable candidata:

```text
F = (K1 + K4) / K0
```

- K1 inclus le travail de la fourmi. Donc on ne compte 
pas K2-K3 qui sont des fonctions appelées par K1.
- K4 est un candidat naturel de parallélisation par grilla.
- K0 est le temps total 
dur kernel de simulation par itérqtion, donc on le prend comme référence pour la fraction paralelizable.
- K5 est en generale
petite et a parties 
sérialisables (swap de buffers), donc on ne le compte pas dans F.
Qu moins d'être paralelizable, plus F se acerca a 0, limitando el speedup.

Estimación teórica (Amdahl):

```text
S(p) = 1 / ((1 - F) + F/p)
```

Calcular `S(p)` para: `p = {2, 4, 8, 16}`.

## 5) Interpretación esperada para la paralelización

El informe debe incluir:

- Top 2 hotspots (en `ms/iter` y `% kernel`).
- Justificación: por qué son candidatos para OpenMP (o por qué limitan el speedup).
- Distinción clara entre:
  - Tiempo de cálculo útil (`K0`, subpartes `K1..K5`).
  - Costes no directamente paralelizables/perturbadores (`R0`, `E0`).

Regla de lectura:

- Si `R0` domina, hay que aislar el render para cualquier comparación de rendimiento del kernel.
- Si `K1` domina, el bucle sobre hormigas es el primer candidato para paralelización.
- Si `K4` pesa mucho, la evaporación es un candidato natural de paralelización por grilla.

## 6) Riesgos de validez y controles

- **Sesgo por render/VSync**: `R0` puede enmascarar las ganancias del kernel.
  - Control: reporte separado kernel vs render.
- **Efectos transitorios del inicio**: creación de textura y entrada en régimen.
  - Control: warm-up de 200 iteraciones.
- **Ruido del sistema** (SO/virtualización/carga de la máquina).
  - Control: 5 repeticiones + desviación estándar.
- **Mezcla de costes one-shot e iterativos**.
  - Control: `P0-P2` reportados por separado de `ms/iter`.
- **Incoherencia interna de timings**.
  - Control: verificar que `K1 + K4 + K5` aproxime `K0` (error relativo bajo).

## 7) Checklist de entrega

- [ ] Build release ejecutado (`make clean && make all`).
- [ ] Baseline ejecutada con `OMP_NUM_THREADS=1`.
- [ ] 5 repeticiones realizadas con 1200 iteraciones cada una.
- [ ] 200 iteraciones de warm-up excluidas en cada repetición.
- [ ] Tabla A completada (`ms/iter`, desviación estándar, `% kernel`).
- [ ] Tabla B completada (`T_iter_total`, `T_kernel`, `T_render`, `hormigas/s`).
- [ ] `F = (K1+K4)/K0` calculada.
- [ ] `S(p)` (Amdahl) calculada para `p={2,4,8,16}`.
- [ ] Top 2 hotspots identificados y comentados.
- [ ] Configuración de hardware/software reportada (CPU, núcleos, SO, WSL2).
