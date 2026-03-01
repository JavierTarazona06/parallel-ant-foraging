# Parallel Ant Foraging

C++ simulator of ant colony optimization on fractal landscapes, with vectorized data layout and OpenMP parallelism.

## Requirements

- Linux toolchain with C++17 support (`g++`, `make`)
- SDL2 development headers

```bash
sudo apt update
sudo apt install -y build-essential libsdl2-dev
```

## Build

### Release

```bash
cd ~/parallele/parallel-ant-foraging/src
make clean
make all
```

### Debug

```bash
cd ~/parallele/parallel-ant-foraging/src
make clean
make DEBUG=yes all
```

## Run

Baseline (`AoS`) is the default layout.

### Interactive

```bash
cd ~/parallele/parallel-ant-foraging/src

# Baseline AoS (default)
./ant_simu.exe

# Baseline AoS (explicit)
./ant_simu.exe --layout aos

# SoA layout
./ant_simu.exe --layout soa
```

### Benchmark

```bash
cd ~/parallele/parallel-ant-foraging/src

# Baseline AoS
./ant_simu.exe --benchmark --layout aos --iterations 1200 --warmup 200

# SoA layout
./ant_simu.exe --benchmark --layout soa --iterations 1200 --warmup 200
```

## Measure Execution Time

See [`docs/instructions/measure_temps.md`](docs/instructions/measure_temps.md) for instructions to measure the program's execution time.

TODO:
- Problema del comportamiento aleatorio para
que sea uniforme continuo. Esto es necesario?
