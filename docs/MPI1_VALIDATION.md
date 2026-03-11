# MPI1 Quick Validation (np=1)
Run this short sanity check before MPI scaling.
```bash
make -C src clean all
./src/ant_simu.exe --exec serial --layout soa --benchmark --no-render --iterations 1200 --warmup 200 > /tmp/serial_soa.metrics 2>/tmp/serial_soa.stderr
mpirun -np 1 ./src/ant_simu.exe --exec mpi1 --layout soa --benchmark --no-render --iterations 1200 --warmup 200 > /tmp/mpi1_np1.metrics 2>/tmp/mpi1_np1.stderr
grep '^METRIC ' /tmp/serial_soa.metrics | head
grep '^METRIC ' /tmp/mpi1_np1.metrics | head
```
Checklist:
- Both runs finish without crash (exit code 0).
- `METRIC` lines exist (`k0_ns`, `k1_ns`, `k4_ns`, `k5_ns`).
- MPI `np=1` prints one metric set (root-only output behavior).
- `food_quantity` grows or stays monotonic (check invariant output if enabled).
- Pheromones stay in expected range (typically `[0,1]`; check invariant output if enabled).
- `serial+soa` and `mpi1+soa np=1` timings are in the same order of magnitude (not necessarily identical).
