#pragma once

#include <cstddef>
#include <cstdint>

namespace mpi_runtime {

// Initialize MPI only when the selected execution mode needs distributed processes.
void init(int* argc, char*** argv);
// Finalize MPI safely after the last distributed step finishes.
void finalize();

// Query the current rank and communicator size through a serial-safe wrapper.
int rank();
int size();
bool is_root();

// Expose the collectives used by MPI backends behind simple typed helpers.
void allreduce_max_double_array(double* ptr, std::size_t n);
std::uint64_t allreduce_sum_uint64(std::uint64_t value);
std::uint64_t allreduce_max_uint64(std::uint64_t value);
double allreduce_max_double(double value);

} // namespace mpi_runtime
