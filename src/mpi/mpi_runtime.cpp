#include "mpi_runtime.hpp"

#include <cassert>
#include <limits>

#include <mpi.h>

namespace mpi_runtime {
namespace {

bool mpi_is_initialized()
{
    int initialized = 0;
    MPI_Initialized(&initialized);
    return initialized != 0;
}

bool mpi_is_finalized()
{
    int finalized = 0;
    MPI_Finalized(&finalized);
    return finalized != 0;
}

int checked_count(std::size_t n)
{
    assert(n <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
    return static_cast<int>(n);
}

} // namespace

void init(int* argc, char*** argv)
{
    if (!mpi_is_initialized()) {
        MPI_Init(argc, argv);
    }
}

void finalize()
{
    if (mpi_is_initialized() && !mpi_is_finalized()) {
        MPI_Finalize();
    }
}

int rank()
{
    if (!mpi_is_initialized() || mpi_is_finalized()) {
        return 0;
    }
    int r = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &r);
    return r;
}

int size()
{
    if (!mpi_is_initialized() || mpi_is_finalized()) {
        return 1;
    }
    int s = 1;
    MPI_Comm_size(MPI_COMM_WORLD, &s);
    return s;
}

bool is_root()
{
    return rank() == 0;
}

void allreduce_max_double_array(double* ptr, std::size_t n)
{
    if (ptr == nullptr || n == 0 || !mpi_is_initialized() || mpi_is_finalized()) {
        return;
    }
    MPI_Allreduce(MPI_IN_PLACE, ptr, checked_count(n), MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
}

std::uint64_t allreduce_sum_uint64(std::uint64_t value)
{
    if (!mpi_is_initialized() || mpi_is_finalized()) {
        return value;
    }
    unsigned long long local = static_cast<unsigned long long>(value);
    unsigned long long global = 0ULL;
    MPI_Allreduce(&local, &global, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    return static_cast<std::uint64_t>(global);
}

double allreduce_max_double(double value)
{
    if (!mpi_is_initialized() || mpi_is_finalized()) {
        return value;
    }
    double global = 0.0;
    MPI_Allreduce(&value, &global, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    return global;
}

} // namespace mpi_runtime

