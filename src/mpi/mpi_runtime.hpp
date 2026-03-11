#pragma once

#include <cstddef>
#include <cstdint>

namespace mpi_runtime {

void init(int* argc, char*** argv);
void finalize();

int rank();
int size();
bool is_root();

void allreduce_max_double_array(double* ptr, std::size_t n);
std::uint64_t allreduce_sum_uint64(std::uint64_t value);
std::uint64_t allreduce_max_uint64(std::uint64_t value);
double allreduce_max_double(double value);

} // namespace mpi_runtime
