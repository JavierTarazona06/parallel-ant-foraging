# Project Structure

## 1) Purpose of this document
This file explains the architecture of the project in a practical way:
- what each folder is for,
- what the backend files are,
- and how the execution modes (`serial`, `omp`, `mpi1`, `mpi2`) are wired.

It is a quick guide for teammates before reading source code in detail.

## 2) High-level layout

```text
parallel-ant-foraging/
├── src/                  # C++ source code (simulation, backends, MPI, rendering)
├── scripts/              # benchmark/measurement + plot generation scripts
├── docs/                 # project docs + report sources
├── results/              # measurement outputs (csv, metrics, logs)
└── README.md
```

## 3) Core architecture (by layer)

### 3.1 Entry + orchestration
- `src/ant_simu.cpp`
  - program entrypoint (`main`)
  - parses config, initializes world, selects backend, starts runner
  - handles MPI init/finalize when `exec` is MPI mode

### 3.2 CLI / configuration
- `src/app/cli.hpp`
  - config structs (`RunConfig`, `SimConfig`)
  - enum definitions (`ExecModel`, `AntLayout`, `InitMode`)
- `src/app/cli.cpp`
  - parses CLI flags (`--exec`, `--layout`, `--benchmark`, etc.)
  - prints usage/help and validates arguments

### 3.3 Backend abstraction (important)
- `src/sim/backend.hpp`
  - defines backend interface (`name()`, `step(...)`, `create_renderer(...)`)
  - defines `WorldState` shared by all backends
- `src/sim/backend.cpp`
  - factory/selector (`make_backend(...)`)
  - routes mode+layout to concrete backend class

### 3.4 Simulation runner (loops)
- `src/sim/runner.hpp`
  - benchmark totals/metrics structures
- `src/sim/runner.cpp`
  - `run_benchmark(...)`: fixed-iteration benchmark + METRIC output
  - `run_interactive(...)`: event loop + render loop
  - MPI reductions for wall-time metrics

### 3.5 Data model / kernels
- `src/fractal_land.hpp/.cpp`
  - terrain generation and access cost map
- `src/pheronome.hpp` (+ alias `src/pheromone.hpp`)
  - pheromone storage and operations (mark, evaporation, update)
- `src/ant.hpp/.cpp`
  - AoS ant logic
- `src/ants_soa.hpp/.cpp`
  - SoA ant containers and stepping helpers
- `src/soa_ant_step.hpp`
  - shared "one ant step" core for SoA paths

### 3.6 Rendering / UI
- `src/window.hpp/.cpp`
  - SDL window wrapper
- `src/renderer.hpp/.cpp`
  - draws terrain, ants, pheromones

### 3.7 Profiling / timing
- `src/timing_profile.hpp/.cpp`
  - timing sections + utilities used in benchmark metrics

### 3.8 MPI utilities
- `src/mpi/mpi_runtime.hpp/.cpp`
  - small MPI wrapper (`init`, `finalize`, `rank`, `size`, collectives)

### 3.9 MPI2 local grid helper
- `src/mpi2/local_pheromone_grid.hpp/.cpp`
  - local subgrid with halo cells for mpi2 backend

## 4) Backend files (what each one does)

### Serial backends
- `src/sim/aos_backend.hpp/.cpp`
  - serial execution with AoS data layout
- `src/sim/soa_backend.hpp/.cpp`
  - serial execution with SoA data layout

### OpenMP backend
- `src/sim/omp_soa_backend.hpp/.cpp`
  - SoA + OpenMP path
  - parallel ant loop + thread-local touched buffers + merge/replay strategy

### MPI1 backend
- `src/sim/mpi1_soa_backend.hpp/.cpp`
  - full map per rank
  - ants partitioned by index range
  - pheromone sync by `Allreduce(MAX)` (frequency controlled by config)

### MPI2 backend
- `src/sim/mpi2_soa_backend.hpp/.cpp`
  - domain decomposition (row blocks per rank)
  - halo exchange + ant migration between neighbors
  - local subdomain update with `LocalPheromoneGrid`

## 5) Mode routing map

Implemented in `src/sim/backend.cpp`:

```text
serial + aos -> AosBackend
serial + soa -> SoaBackend
omp    + soa -> OmpSoaBackend
mpi1   + soa -> Mpi1SoaBackend
mpi2   + soa -> Mpi2SoaBackend
```

Unsupported combinations fail with a clear error (example: `omp + aos`).

## 6) Scripts and report integration

- `scripts/measure_openmp.sh` -> OpenMP sweep
- `scripts/measure_mpi1.sh`   -> MPI1 sweep
- `scripts/measure_mpi2.sh`   -> MPI2 sweep
- `scripts/generate_*_report_plots.py` -> report figures from summary CSV

Report sources:
- `docs/report/sections/*.tex`
- main report file: `docs/report/report.tex`

## 7) Quick navigation tips

If you want to understand runtime behavior quickly:
1. start with `src/ant_simu.cpp`
2. then `src/app/cli.*`
3. then `src/sim/backend.*` and the selected backend file
4. then `src/sim/runner.cpp`
5. finally, dive into kernels (`ants_soa.cpp`, `soa_ant_step.hpp`, `pheronome.hpp`)

