# Execution Flow (Current)

## 1) Purpose
This file explains the real runtime flow of the project in a simple way.
It is meant for presentations with teammates/tutors: command -> `main()` -> backend -> kernels -> metrics.
It covers all current execution modes:
- `serial` (`aos` and `soa`)
- `omp` (`soa`)
- `mpi1` (`soa`)
- `mpi2` (`soa`; render allowed only with `np=1`)

## 2) Quick Start Commands

### Build
```bash
make -C src clean all
```

### Interactive runs
```bash
# serial + aos
./src/ant_simu.exe --exec serial --layout aos

# serial + soa
./src/ant_simu.exe --exec serial --layout soa

# omp + soa
OMP_NUM_THREADS=8 ./src/ant_simu.exe --exec omp --layout soa --threads 8
```

### Benchmark runs (no render)
```bash
# serial + soa benchmark
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
./src/ant_simu.exe --benchmark --no-render --exec serial --layout soa \
  --iterations 1200 --warmup 200

# omp + soa benchmark
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy OMP_NUM_THREADS=8 OMP_DYNAMIC=FALSE \
./src/ant_simu.exe --benchmark --no-render --exec omp --layout soa --threads 8 \
  --iterations 1200 --warmup 200

# mpi1 + soa benchmark
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy OMP_NUM_THREADS=1 OMP_DYNAMIC=FALSE \
mpirun --bind-to core --map-by core -np 4 ./src/ant_simu.exe \
  --benchmark --no-render --exec mpi1 --layout soa --mpi-sync-every 1 \
  --iterations 1200 --warmup 200

# mpi2 + soa benchmark (required mode)
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy OMP_NUM_THREADS=1 OMP_DYNAMIC=FALSE \
mpirun --bind-to core --map-by core -np 4 ./src/ant_simu.exe \
  --benchmark --no-render --exec mpi2 --layout soa \
  --iterations 1200 --warmup 200
```

## 3) Main CLI Flags

| Flag | Purpose | Default | Parsed in |
|---|---|---|---|
| `--exec serial|omp|mpi1|mpi2` | Execution model | `serial` | `src/app/cli.cpp` |
| `--layout aos|soa` | Ant storage layout | `aos` | `src/app/cli.cpp` |
| `--benchmark` | Benchmark loop + METRIC output | `false` | `src/app/cli.cpp` |
| `--no-render` | Disable SDL rendering | render ON | `src/app/cli.cpp` |
| `--iterations N` | Total iterations | `1200` | `src/app/cli.cpp` |
| `--warmup N` | Warmup iterations (not measured) | `200` | `src/app/cli.cpp` |
| `--threads N` | OpenMP thread count (`exec=omp`) | unset | `src/app/cli.cpp` |
| `--mpi-sync-every K` | MPI1 pheromone sync frequency | `1` | `src/app/cli.cpp` |
| `--ants --seed --alpha --beta --epsilon --init` | Simulation params | see defaults | `src/app/cli.cpp` |

## 4) Global Program Flow

```text
./src/ant_simu.exe ...                       # user command
-> main() [src/ant_simu.cpp]                # entrypoint
-> parse_args() [src/app/cli.cpp]           # read CLI into RunConfig/SimConfig
-> if exec in {mpi1,mpi2}: mpi_runtime::init()  # initialize MPI runtime
-> mpi2 guard checks                          # require benchmark, force no-render
-> configure_thread_count()                  # only meaningful for OMP
-> SDL_Init()                                # initialize SDL
-> print_startup_header()                    # INFO or METRIC header (root-only for MPI)
-> initialize world data                     # land, ants, pheromone map
-> make_backend() [src/sim/backend.cpp]      # backend selection
-> run_benchmark() or run_interactive()      # loop selection
-> backend.step(...)                          # one iteration kernel path
-> finalize                                   # SDL_Quit + optional mpi_runtime::finalize()
```

## 5) Backend Routing

Routing is done in `src/sim/backend.cpp`:

```text
exec=serial + layout=aos -> AosBackend       # serial AoS path
exec=serial + layout=soa -> SoaBackend       # serial SoA path
exec=omp    + layout=soa -> OmpSoaBackend    # OpenMP path
exec=mpi1   + layout=soa -> Mpi1SoaBackend   # MPI1 path
exec=mpi2   + layout=soa -> Mpi2SoaBackend   # MPI2 path
```

Invalid combinations fail with a clear message (for example `omp+aos`, `mpi1+aos`, `mpi2+aos`).

## 6) Per-Mode Runtime Flows

### A) `serial + aos`

```text
main()                                       # orchestration
-> make_backend() -> AosBackend              # backend select
-> run_* loop                                # benchmark or interactive
-> AosBackend::step()                        # one iteration
-> for each ant: ant.advance(...)            # movement/decision
-> phen.mark_pheronome(...)                  # deposit
-> phen.do_evaporation()                     # K4
-> phen.update()                             # K5
```

### B) `serial + soa`

```text
main()
-> make_backend() -> SoaBackend
-> run_* loop
-> SoaBackend::step()
-> advance_time_soa(...)                     # SoA driver
-> advance_ants_soa_range(...)               # ant loop by index
-> advance_one_ant_soa_core(...)             # shared core ant logic
-> mark sink -> phen.mark_pheronome(...)     # direct global mark
-> phen.do_evaporation()                     # K4
-> phen.update()                             # K5
```

### C) `omp + soa`

```text
main()
-> configure_thread_count()                  # --threads / OMP_NUM_THREADS
-> phen.set_openmp_evaporation_enabled(true) # only for OMP mode
-> make_backend() -> OmpSoaBackend
-> run_* loop
-> OmpSoaBackend::step()
-> #pragma omp parallel + omp for            # ant loop parallelized
-> advance_one_ant_soa_core(...)             # same core as serial SoA
-> thread-local touched lists                # no global pheromone writes in region
-> merge + sort + unique                     # sparse merge stage
-> replay mark_pheronome(...)                # sequential replay
-> phen.do_evaporation()                     # OpenMP evaporation path enabled
-> phen.update()
```

### D) `mpi1 + soa`

```text
main()
-> mpi_runtime::init()                       # MPI init
-> make_backend() -> Mpi1SoaBackend
-> run_* loop (root-only METRIC print)
-> Mpi1SoaBackend::step()
-> split ant index range by rank             # [begin,end) block per rank
-> advance_ants_soa_range(...)               # local ants only
-> Allreduce(SUM) food_delta                 # global food consistency
-> phen.do_evaporation() + phen.update()     # full map locally
-> every K iterations:                        # K = --mpi-sync-every
-> Allreduce(MAX) on V1 and V2 full arrays   # pheromone reconcile
```

### E) `mpi2 + soa`

```text
main()
-> mpi_runtime::init()
-> if render and np>1: error                  # render allowed only in single-process mpi2
-> make_backend() -> Mpi2SoaBackend
-> run_benchmark() or run_interactive()       # benchmark and interactive are both valid
-> Mpi2SoaBackend::step()
-> initialize 1D row partition if needed     # y0..y1 ownership per rank
-> halo_exchange() for V1/V2                 # MPI_Sendrecv with up/down
-> local_k5_update()                          # local interior update using halo
-> local_k4_evaporation()                     # local interior evaporation
-> step_local_ants()                          # only local ant list
-> detect boundary crossing -> outboxes       # deferred migration policy
-> Allreduce(SUM) food_delta                  # global food metric
-> exchange_migrating_ants()                  # counts + packed buffers
```

## 7) Shared SoA Core and Why It Matters

Both `serial+soa` and `omp+soa` use the same ant core function:

```text
advance_one_ant_soa_core(...) [src/soa_ant_step.hpp]
```

This keeps movement logic aligned between serial and OMP.
The key difference is the mark sink:
- serial SoA writes marks directly,
- OMP records touched cells and replays later to avoid races.

## 8) Where METRIC Lines Are Produced

```text
main() -> print_startup_header()             # startup METRIC fields
run_benchmark() [src/sim/runner.cpp]         # per-run totals and checks
```

Important behavior:
- For MPI modes, runner reduces times with `MPI_Allreduce(MAX)` (wall-time).
- For MPI modes, only root rank prints final `METRIC ...` lines.
- `mpi2` adds `k_mpi_halo_ns` and `k_mpi_migrate_ns` metrics.

## 9) Measurement Scripts (Project Rhythm)

```bash
# OpenMP thread sweep
./scripts/measure_openmp.sh --folder test_openmp_report_local --layout soa --exec omp \
  --threads 1,2,4,8 --iterations 1200 --warmup 200 --reps 7

# MPI1 process sweep
./scripts/measure_mpi1.sh --folder test_mpi1_sync_every_rerun --np 1,2,4,8 \
  --iterations 400 --warmup 100 --reps 3

# MPI2 process sweep
./scripts/measure_mpi2.sh --folder test_mpi2_report_v1 --np 1,2,4,8 \
  --iterations 400 --warmup 100 --reps 3
```

Typical result path:

```text
results/<folder>/<exec>/<layout>/<timestamp>/...
```

## 10) Quick Troubleshooting

- `exec=mpi2` with render and `np>1`: program exits with clear error.
- `exec=mpi2` render works only for single process (`-np 1`).
- `omp+aos`, `mpi1+aos`, `mpi2+aos`: rejected by backend factory.
- If MPI command fails, verify `mpirun` and use same executable path as build output.
- If timing output is duplicated in MPI, verify you are reading root output only.

## 11) File Map (Execution-Critical)

- `src/ant_simu.cpp`: top-level orchestration (`main()`), mode guards, startup flow.
- `src/app/cli.hpp` + `src/app/cli.cpp`: argument parsing and config defaults.
- `src/sim/backend.cpp`: backend routing by `exec/layout`.
- `src/sim/runner.cpp`: benchmark/interactive loops and `METRIC` aggregation.
- `src/sim/aos_backend.cpp`: serial AoS iteration path.
- `src/sim/soa_backend.cpp`: serial SoA iteration path.
- `src/sim/omp_soa_backend.cpp`: OpenMP SoA path.
- `src/sim/mpi1_soa_backend.cpp`: MPI1 path (full-map sync by MAX).
- `src/sim/mpi2_soa_backend.cpp`: MPI2 path (partition + halo + migration).
- `src/pheronome.hpp`: pheromone storage, mark/evaporation/update, contiguous buffers.
- `src/mpi/mpi_runtime.hpp` + `src/mpi/mpi_runtime.cpp`: MPI init/finalize + collective wrappers.
