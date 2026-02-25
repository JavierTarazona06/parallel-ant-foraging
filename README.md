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

```bash
cd ~/parallele/parallel-ant-foraging/src
./ant_simu.exe
```

## Measure Execution Time

See [`docs/instructions/measure_temps.md`](docs/instructions/measure_temps.md) for instructions to measure the program's execution time.
