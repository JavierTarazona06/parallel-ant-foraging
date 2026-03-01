# Plan 5.1 — Vectorización (SoA) manteniendo baseline AoS (1 binario, 2 layouts)

## Resumen
Implementar el punto **5.1 Vectorisation** del `docs/context/Context.pdf` cambiando el layout de datos de las hormigas a **SoA (Structure of Arrays)**, manteniendo además el layout actual **AoS (std::vector<ant>)** seleccionable por CLI para poder comparar dentro del mismo binario.  
Incluye explícitamente:
- SoA (arrays de `x/y`, `state`, `seed`).
- Adaptación de consumidores (principalmente `Renderer`).
- Arreglo del RNG/seed por hormiga (bug actual: `m_seed` no se inicializa).
- Tipos compactos en SoA (`state:uint8_t`, `seed:uint32_t`, `x/y:int32_t`).
- Mantener `SDL_Point` fuera del hot loop (simulación): solo convertir para render.

## Alcance / No alcance
- En alcance: refactor de datos de hormigas + integración en `ant_simu`, render, compat AoS/SoA, mantener métricas/benchmark.
- Fuera de alcance (por ahora): micro-steps/compactación de activas, OpenMP (5.2), cambios en el modelo o en la distribución del RNG (se mantiene igual según decisión).

## Estrategia elegida: 1 binario, 2 layouts (recomendado)
- **AoS (baseline)**: se mantiene `std::vector<ant>` y será el **default**.
- **SoA (nuevo)**: se agrega una implementación paralela seleccionable por CLI con `--layout soa`.
- Objetivo: comparar AoS vs SoA con el **mismo ejecutable** y los mismos parámetros.

---

## Cambios de interfaces (decisión completa)

### 1) CLI (nuevo flag)
Archivo: `src/ant_simu.cpp#L21` (RunConfig/parse_args)
- Agregar `--layout <aos|soa>` (y opcional `--layout=aos|soa`).
- **Default**: `aos` para no romper el flujo existente de medición (`scripts/measure_temps.sh`).
- Actualizar `print_usage()` (`src/ant_simu.cpp#L43`) para documentar el flag.

### 2) Tipo nuevo SoA
Nuevo archivo: `src/ants_soa.hpp` (nuevo)
- Definir:
  - `struct AntsSoA { std::vector<int32_t> x, y; std::vector<uint8_t> state; std::vector<uint32_t> seed; ... }`
  - Invariantes: todos los arrays tienen el mismo tamaño; `state[i] ∈ {0,1}`.
  - Helpers: `reserve(n)`, `size()`, `push_back(x,y,seed,state)`.

Nuevo archivo: `src/ants_soa.cpp` (nuevo)
- Implementar el kernel SoA (ver sección “Kernel SoA”).

### 3) Renderer compatible con ambos layouts
Archivos: `src/renderer.hpp#L7`, `src/renderer.cpp#L5`
- Mantener constructor actual para AoS:
  - `Renderer(..., const std::vector<ant>& ants)`
- Agregar segundo constructor para SoA:
  - `Renderer(..., const AntsSoA& ants)`
- Internamente, `Renderer` guarda puntero a AoS **o** a SoA, y en `display()` (`src/renderer.cpp#L23`) hace un branch:
  - AoS: comportamiento actual.
  - SoA: iterar `for i in [0..ants.size)` y `win.pset(ants.x[i], ants.y[i])`.
- Nota: la conversión a `SDL_Point` (si se usa) queda confinada al render; el hot loop de simulación no lo usa.

### 4) pheronome: overload para evitar `SDL_Point` en hot loop
Archivo: `src/pheronome.hpp#L73`
- Reescribir para tener overload sin `SDL_Point`:
  - `void mark_pheronome(std::int32_t x, std::int32_t y);`
  - `void mark_pheronome(const position_t& pos)` queda como wrapper que llama al overload por `x/y`.
- El kernel SoA llama siempre a `mark_pheronome(x,y)`.

### 5) RNG: seeds por hormiga + overloads compactos (sin cambiar comportamiento)
Archivo: `src/rand_generator.hpp#L24`
- Mantener funciones actuales (por compat con AoS).
- Agregar overloads:
  - `rand_int32(min,max,uint32_t& seed)`
  - `rand_double(min,max,uint32_t& seed)`
- Implementación debe **replicar** la fórmula actual:
  - `seed = (1664525 * seed + 1013904223) % 0xFFFFFFFF;`
  - `rand_double` se mantiene tal cual (aunque sea discreto) por decisión.

### 6) Fix AoS: inicializar `m_seed`
Archivo: `src/ant.hpp#L16`
- Cambiar constructor a:
  - `ant(..., std::size_t seed) : m_state(unloaded), m_position(pos), m_seed(seed) {}`
- Objetivo: que el layout AoS sea definido/determinista y comparable.

---

## Kernel SoA (detalle implementable)

### Función principal (por iteración)
Nuevo archivo: `src/ants_soa.cpp`
- `void advance_time_soa(const fractal_land& land, pheronome& phen,`
  `int32_t nest_x, int32_t nest_y, int32_t food_x, int32_t food_y,`
  `AntsSoA& ants, double eps, std::size_t& food_counter, IterTimingNs* iter_timing)`
- Debe replicar la estructura de `advance_time()` existente (`src/ant_simu.cpp#L103`):
  - Medir `t0/t1` (avance de todas las hormigas), luego `do_evaporation()`, luego `update()`.
  - Acumular `k1/k4/k5` en `iter_timing` igual que AoS.

### Función por hormiga (lógica idéntica a `ant::advance`)
Dentro de `src/ants_soa.cpp`, helper:
- `static inline void advance_one_ant_soa(...)`
- Por hormiga `i`:
  - Variables locales:
    - `uint32_t& seed = ants.seed[i];`
    - `uint8_t& state = ants.state[i];` (`0=unloaded`, `1=loaded`)
    - `int32_t x = ants.x[i], y = ants.y[i];`
    - `double consumed_time = 0;`
  - `while (consumed_time < 1.)`:
    - Medición fina igual a `src/ant.cpp#L16-L67`:
      - `ant_step_start_ns`
      - `before_mark_ns` → `profile_add_k2(...)`
      - `after_mark_ns` → `profile_add_k3(...)`
      - `ant_step_end_ns` → `profile_add_k2(...)`
    - `ind_pher = state` (0/1).
    - `choix = rand_double(0., 1., seed)` (overload u32).
    - Calcular `max_phen` leyendo vecinos con `phen(...)` como en AoS, casteando índices:
      - `phen(static_cast<size_t>(x-1), static_cast<size_t>(y))[ind_pher]`, etc.
    - Rama exploración vs explotación igual a `src/ant.cpp#L26-L46`:
      - Exploración: `do { pick dir; nx,ny } while (phen(size_t(nx),size_t(ny))[ind_pher] == -1)`
      - Explotación: elegir vecino con `== max_phen`
    - `consumed_time += land(static_cast<unsigned long>(nx), static_cast<unsigned long>(ny));`
    - `phen.mark_pheronome(nx, ny);`
    - Actualizar `x/y`.
    - Transiciones:
      - Si `(x==nest_x && y==nest_y)`:
        - Si `state==1` entonces `food_counter++`.
        - `state=0`.
      - Si `(x==food_x && y==food_y)` entonces `state=1`.
  - Guardar `ants.x[i]=x; ants.y[i]=y;`.

---

## Integración en `ant_simu.cpp` (decisión completa)

Archivo: `src/ant_simu.cpp#L21`
1) Ampliar `RunConfig` con:
- `enum class AntLayout { aos, soa };`
- `AntLayout layout{AntLayout::aos};`

2) `parse_args()` (`src/ant_simu.cpp#L63`)
- Parsear `--layout` y validar valores.
- Si inválido: mensaje + `return false`.

3) Inicialización de hormigas (P2)
Archivo: `src/ant_simu.cpp#L169`
- Mantener medición `p2_start_ns/p2_end_ns`.
- Branch por layout:
  - AoS: igual que hoy, pero con `ant` constructor ya arreglado.
  - SoA:
    - `AntsSoA ants; ants.reserve(nb_ants);`
    - En el loop:
      - `int32_t ax = gen_ant_pos(); int32_t ay = gen_ant_pos();`
      - `uint32_t ant_seed = static_cast<uint32_t>(seed);` (estado del generador global tras generar posición)
      - `ants.push_back(ax, ay, ant_seed, /*unloaded=*/0);`

4) Renderer
Archivo: `src/ant_simu.cpp#L180`
- Branch por layout para construir `Renderer` usando el constructor correspondiente.

5) Loop principal (benchmark e interactivo)
Archivo: `src/ant_simu.cpp#L196`
- Reemplazar llamada única a `advance_time(...)` por branch:
  - AoS: `advance_time(...)` existente.
  - SoA: `advance_time_soa(...)`.

---

## Build system
Archivo: `src/Makefile#L26`
- Agregar `ants_soa.o` a la regla de link:
  - `ant_simu.exe : ant.o ants_soa.o fractal_land.o renderer.o window.o timing_profile.o ant_simu.o`

---

## Medición y comparación (criterios de éxito)

### Validación funcional mínima
1) Compila en release: `make -C src clean all`.
2) Benchmark AoS (baseline) sin romper métricas:
- `src/ant_simu.exe --benchmark --layout aos --iterations 1200 --warmup 200`
3) Benchmark SoA:
- `src/ant_simu.exe --benchmark --layout soa --iterations 1200 --warmup 200`
4) Interactivo (opcional): correr ambos layouts sin `--benchmark` y verificar que renderiza y no crashea.

### Validación de métricas
- Formato `METRIC ...` debe permanecer igual (mismos keys) para que `scripts/measure_temps.sh` siga funcionando.
- `K0 = K1+K4+K5` debe seguir cumpliéndose a nivel de cómputo (aprox., por medición).

### Comparación de rendimiento (5.1)
- Ejecutar medición repetida con `scripts/measure_temps.sh` en AoS (baseline).
- Medir SoA con una de estas dos opciones:
  - (A) Ejecutar manualmente 5 repeticiones con `--layout soa` y consolidar.

Éxito 5.1:
- Implementación SoA correcta (layout por arrays + índice de hormiga).
- Renderer y binario soportan ambos layouts.
- Mejoras de tiempo observables en `K1`/`K0` (no garantizado, pero esperado), manteniendo comportamiento del RNG.

---

## Supuestos (explícitos)
- Se mantiene el RNG actual (misma fórmula y misma “discretización” de `rand_double`) por decisión.
- Default `--layout aos` para no alterar el flujo existente.
- `land.dimensions()` cabe en `int32_t` (cierto para la config actual: 513).
- El objetivo es **vectorización por layout (SoA)**, no necesariamente SIMD explícito.
