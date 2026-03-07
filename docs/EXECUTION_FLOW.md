# Application Execution Flow

## 1) Purpose
This document explains how the program executes internally, step by step.
It is meant for quick explanations to teammates/tutors: command -> `main()` -> parsing -> backend -> kernels.
It covers current execution modes: `serial+aos`, `serial+soa`, and `omp+soa`.
It also shows where benchmark `METRIC ...` lines are produced.

## 2) Quick Commands

### Build
```bash
make -C src clean all
```

### Interactive runs
```bash
# Serial + AoS
./src/ant_simu.exe --exec serial --layout aos

# Serial + SoA
./src/ant_simu.exe --exec serial --layout soa

# OpenMP + SoA
OMP_NUM_THREADS=8 ./src/ant_simu.exe --exec omp --layout soa --threads 8
```

### Benchmark runs (no render)
```bash
# Serial + SoA (baseline)
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
./src/ant_simu.exe --benchmark --no-render --exec serial --layout soa \
  --iterations 1200 --warmup 200

# OpenMP + SoA (thread scaling)
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy OMP_NUM_THREADS=8 \
./src/ant_simu.exe --benchmark --no-render --exec omp --layout soa \
  --threads 8 --iterations 1200 --warmup 200
```

## 3) Key CLI Flags (summary)

| Flag | What it does | Default | Parsed in |
|---|---|---|---|
| `--exec serial|omp|mpi1|mpi2` | Execution model | `serial` | `src/app/cli.cpp` |
| `--layout aos|soa` | Ant data layout | `aos` | `src/app/cli.cpp` |
| `--benchmark` | Enables benchmark loop + METRIC output | `false` | `src/app/cli.cpp` |
| `--no-render` | Disables rendering | render ON | `src/app/cli.cpp` |
| `--iterations N` | Total loop iterations | `1200` | `src/app/cli.cpp` |
| `--warmup N` | Unmeasured warmup iterations | `200` | `src/app/cli.cpp` |
| `--threads N` | OpenMP thread count (for `--exec omp`) | unset | `src/app/cli.cpp` |
| `--ants --seed --alpha --beta --epsilon --init` | Simulation parameters | see `RunConfig/SimConfig` | `src/app/cli.hpp/.cpp` |

## 4) Global Flow (always)

```text
Terminal command                           # user starts the program
-> ./src/ant_simu.exe                      # executable entry
-> main() [src/ant_simu.cpp]               # top-level orchestration
-> parse_args() [src/app/cli.cpp]          # CLI/config parsing
-> configure_thread_count()                # OpenMP thread setup if needed
-> print_startup_header()                  # INFO or METRIC header lines
-> build world state                       # land + ants + pheromone map
-> make_backend() [src/sim/backend.cpp]    # choose backend from exec/layout
-> run_benchmark() or run_interactive()    # choose execution loop
-> backend.step(...) per iteration         # run simulation kernels
```

## 5) Per-Mode Flow (with arrows)

### A) `--exec serial --layout aos`
```text
main()                                     # start application
-> make_backend() -> AosBackend            # select serial AoS backend
-> run_* loop                              # benchmark or interactive loop
-> AosBackend::step()                      # one simulation iteration
-> for each ant: ant.advance(...)          # ant movement/decision/update
-> mark_pheronome(...)                     # pheromone deposit during ant step
-> do_evaporation()                        # pheromone decay phase
-> update()                                # swap/apply pheromone buffers
```

### B) `--exec serial --layout soa`
```text
main()                                     # start application
-> make_backend() -> SoaBackend            # select serial SoA backend
-> run_* loop                              # benchmark or interactive loop
-> SoaBackend::step()                      # one simulation iteration
-> advance_time_soa(...)                   # SoA iteration driver
-> for each ant index i:                   # iterate ants by index
-> advance_one_ant_soa_core(...)           # shared SoA ant-step core logic
-> mark sink: phen.mark_pheronome(...)     # direct mark to global pheromone
-> do_evaporation()                        # pheromone decay phase
-> update()                                # swap/apply pheromone buffers
```

### C) `--exec omp --layout soa`
```text
main()                                     # start application
-> configure_thread_count()                # apply --threads / OMP_NUM_THREADS
-> phen.set_openmp_evaporation_enabled(1)  # enable OMP evaporation path
-> make_backend() -> OmpSoaBackend         # select OpenMP SoA backend
-> run_* loop                              # benchmark or interactive loop
-> OmpSoaBackend::step()                   # one simulation iteration
-> omp parallel + omp for over ants        # parallel ant loop
-> advance_one_ant_soa_core(...)           # same shared SoA core logic
-> thread-local touched_v1/touched_v2      # record touched cells (no global map write)
-> end omp region                           # synchronize threads
-> merge + sort + unique touched cells     # sparse reduction stage
-> replay mark_pheronome(...)              # apply marks sequentially once per cell
-> do_evaporation()                        # evaporation (OMP-enabled path)
-> update()                                # swap/apply pheromone buffers
```

## 6) Important Recent Change (SoA serial vs SoA OMP)

Both `serial+soa` and `omp+soa` now use the same ant-step core:

```text
advance_one_ant_soa_core(...) [src/soa_ant_step.hpp]
```

Main difference now:
- Serial SoA: direct pheromone marking.
- OMP SoA: thread-local touched-cell recording, then merge/replay outside the parallel region.

This keeps the movement logic aligned while still preventing races in OpenMP mode.

## 7) Where Metrics Are Printed

```text
main() -> print_startup_header()           # startup metadata
-> INFO ...                                 # non-benchmark mode
-> METRIC exec/layout/threads/...           # benchmark mode header

run_benchmark() [src/sim/runner.cpp]       # benchmark totals
-> METRIC k0_ns, k1_ns, k2_ns, k3_ns, ...  # kernel timing outputs
-> METRIC touched_* and correctness checks  # sparse + validation metrics
```

Quick kernel meaning:
- `K0`: total `backend.step(...)` wall-time per iteration.
- `K1`: ant-update phase.
- `K4`: pheromone evaporation phase.
- `K5`: pheromone update/swap phase.

## 8) Useful Scripts for Project Workflow

```bash
# OpenMP thread sweep
./scripts/measure_openmp.sh --folder test_openmp_sweep --layout soa --exec omp \
  --threads 1,2,4,8 --iterations 1200 --warmup 200 --reps 5

# Serial baseline measurements
./scripts/measure_temps.sh --folder test_time_measurements --layout soa
./scripts/measure_temps.sh --folder test_time_measurements --layout aos
```

Typical output paths:
- `results/<folder>/<exec>/<layout>/<timestamp>/...` (OpenMP sweep)
- `results/<folder>/<layout>/<timestamp>/...` (serial baseline script)

## 9) Practical Notes

- `--exec omp --layout aos` is currently rejected by the backend factory.
- `mpi1/mpi2` are parsed by CLI, but still not implemented in backend factory.
- If behavior changes, verify first in:
  - `src/ant_simu.cpp` (main orchestration)
  - `src/sim/backend.cpp` (backend routing)
  - `src/sim/runner.cpp` (run loops + METRIC output)
