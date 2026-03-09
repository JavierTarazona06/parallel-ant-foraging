#include "mpi2_soa_backend.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include <mpi.h>

#include "../mpi/mpi_runtime.hpp"
#include "../renderer.hpp"
#include "../timing_profile.hpp"

namespace {

constexpr std::size_t kHaloWidth = 1u;

bool mpi2_debug_partition_enabled()
{
    const char* env = std::getenv("MPI2_DEBUG_PARTITION");
    if (env == nullptr) {
        return false;
    }
    const std::string value(env);
    return value == "1" || value == "true" || value == "TRUE" || value == "yes" || value == "YES";
}

} // namespace

Mpi2SoaBackend::Mpi2SoaBackend(AntsSoA& ants)
    : m_ants(ants)
{
}

const char* Mpi2SoaBackend::name() const
{
    return "mpi2+soa";
}

void Mpi2SoaBackend::step(WorldState& world, const SimConfig& sim_config)
{
    // MPI2 skeleton: we only initialize and validate the 1D row partition.
    initialize_partition_if_needed(world);

    const std::uint64_t halo_start_ns = profile_now_ns();
    halo_exchange();
    const std::uint64_t halo_end_ns = profile_now_ns();

    if (world.iter_timing != nullptr) {
        world.iter_timing->k_mpi_halo_ns += (halo_end_ns - halo_start_ns);
    }

    (void)sim_config;
}

std::unique_ptr<Renderer> Mpi2SoaBackend::create_renderer(const fractal_land& land,
                                                          const pheronome& phen,
                                                          const position_t& pos_nest,
                                                          const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}

void Mpi2SoaBackend::initialize_partition_if_needed(const WorldState& world)
{
    if (m_partition_ready) {
        if (!m_local_grid_ready) {
            initialize_local_pheromone_grid(world);
        }
        return;
    }

    m_global_w = static_cast<std::size_t>(world.land.dimensions());
    m_global_h = static_cast<std::size_t>(world.land.dimensions());

    m_size = std::max(1, mpi_runtime::size());
    m_rank = std::clamp(mpi_runtime::rank(), 0, m_size - 1);

    const std::size_t rank = static_cast<std::size_t>(m_rank);
    const std::size_t size = static_cast<std::size_t>(m_size);
    const std::size_t base_rows = m_global_h / size;
    const std::size_t remainder = m_global_h % size;

    m_y0 = rank * base_rows + std::min(rank, remainder);
    const std::size_t local_rows = base_rows + (rank < remainder ? 1u : 0u);
    m_y1 = m_y0 + local_rows;

    m_up_rank = (m_rank > 0) ? (m_rank - 1) : MPI_PROC_NULL;
    m_down_rank = (m_rank + 1 < m_size) ? (m_rank + 1) : MPI_PROC_NULL;

    m_partition_ready = true;
    initialize_local_pheromone_grid(world);
    maybe_print_partition_debug();
}

void Mpi2SoaBackend::initialize_local_pheromone_grid(const WorldState& world)
{
    if (!m_partition_ready || m_local_grid_ready) {
        return;
    }

    const std::size_t local_h = m_y1 - m_y0;
    m_local_phen.reset(m_global_w, local_h, -1.0);

    for (std::size_t gy = m_y0; gy < m_y1; ++gy) {
        const std::size_t ly = 1u + (gy - m_y0);
        for (std::size_t gx = 0; gx < m_global_w; ++gx) {
            const std::size_t lx = 1u + gx;
            const auto cell = world.phen(gx, gy);
            m_local_phen.v1(lx, ly) = cell[0];
            m_local_phen.v2(lx, ly) = cell[1];
        }
    }

    m_local_grid_ready = true;
}

void Mpi2SoaBackend::halo_exchange()
{
    if (!m_partition_ready || !m_local_grid_ready) {
        return;
    }
    if (m_local_phen.local_height() == 0u) {
        return;
    }

    halo_exchange_channel(m_local_phen.v1_data(), 100);
    halo_exchange_channel(m_local_phen.v2_data(), 200);
}

void Mpi2SoaBackend::halo_exchange_channel(double* channel_base, int tag_base)
{
    if (channel_base == nullptr) {
        return;
    }

    const std::size_t local_h = m_local_phen.local_height();
    const std::size_t stride = m_local_phen.stride();
    assert(stride <= static_cast<std::size_t>(std::numeric_limits<int>::max()));

    const int row_count = static_cast<int>(stride);
    double* top_halo = channel_base;
    double* first_interior = channel_base + stride;
    double* last_interior = channel_base + local_h * stride;
    double* bottom_halo = channel_base + (local_h + 1u) * stride;

    // 1) send first interior row to UP, receive DOWN first interior row into bottom halo.
    MPI_Sendrecv(first_interior, row_count, MPI_DOUBLE, m_up_rank, tag_base + 1,
                 bottom_halo, row_count, MPI_DOUBLE, m_down_rank, tag_base + 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // 2) send last interior row to DOWN, receive UP last interior row into top halo.
    MPI_Sendrecv(last_interior, row_count, MPI_DOUBLE, m_down_rank, tag_base + 2,
                 top_halo, row_count, MPI_DOUBLE, m_up_rank, tag_base + 2,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

bool Mpi2SoaBackend::within_local_plus_halo_global(std::int32_t x, std::int32_t y) const
{
    if (!m_partition_ready || !m_local_grid_ready) {
        return false;
    }

    // 1D row split: each rank has full X range + left/right halo.
    const std::int32_t x_min = -1;
    const std::int32_t x_max = static_cast<std::int32_t>(m_global_w);
    if (x < x_min || x > x_max) {
        return false;
    }

    // Y range accepts local interior + one halo row above/below.
    const std::int32_t y_min = static_cast<std::int32_t>(m_y0) - 1;
    const std::int32_t y_max = static_cast<std::int32_t>(m_y1); // inclusive halo at m_y1
    return y >= y_min && y <= y_max;
}

Mpi2SoaBackend::LocalCellCoord Mpi2SoaBackend::global_to_local_with_halo(std::int32_t x, std::int32_t y) const
{
    assert(within_local_plus_halo_global(x, y) &&
           "global_to_local_with_halo received coordinates outside local+halo");

    const std::int32_t lx = x + 1;
    const std::int32_t ly = y - static_cast<std::int32_t>(m_y0) + 1;
    assert(lx >= 0 && lx < static_cast<std::int32_t>(m_local_phen.stride()));
    assert(ly >= 0 && ly < static_cast<std::int32_t>(m_local_phen.row_count()));
    return LocalCellCoord{lx, ly};
}

double Mpi2SoaBackend::phen_read_global(std::int32_t x, std::int32_t y, int channel) const
{
    assert(channel == 0 || channel == 1);
    if (!within_local_plus_halo_global(x, y)) {
        assert(false && "phen_read_global outside local+halo");
        return -1.0;
    }

    const LocalCellCoord local = global_to_local_with_halo(x, y);
    if (channel == 0) {
        return m_local_phen.v1(static_cast<std::size_t>(local.lx), static_cast<std::size_t>(local.ly));
    }
    return m_local_phen.v2(static_cast<std::size_t>(local.lx), static_cast<std::size_t>(local.ly));
}

bool Mpi2SoaBackend::mark_pheromone_global(std::int32_t x, std::int32_t y, int channel)
{
    assert(channel == 0 || channel == 1);
    if (!owns_cell_global(x, y)) {
        return false;
    }

    const LocalCellCoord local = global_to_local(x, y);
    if (channel == 0) {
        double& cell = m_local_phen.v1(static_cast<std::size_t>(local.lx), static_cast<std::size_t>(local.ly));
        cell = std::max(cell, 1.0);
    } else {
        double& cell = m_local_phen.v2(static_cast<std::size_t>(local.lx), static_cast<std::size_t>(local.ly));
        cell = std::max(cell, 1.0);
    }
    return true;
}

bool Mpi2SoaBackend::owns_cell_global(std::int32_t x, std::int32_t y) const
{
    if (!m_partition_ready || x < 0 || y < 0) {
        return false;
    }
    const std::size_t gx = static_cast<std::size_t>(x);
    const std::size_t gy = static_cast<std::size_t>(y);
    return gx < m_global_w && gy >= m_y0 && gy < m_y1;
}

Mpi2SoaBackend::LocalCellCoord Mpi2SoaBackend::global_to_local(std::int32_t x, std::int32_t y) const
{
    assert(owns_cell_global(x, y) && "global_to_local called for non-owned cell");

    const std::size_t gx = static_cast<std::size_t>(x);
    const std::size_t gy = static_cast<std::size_t>(y);

    return LocalCellCoord{static_cast<std::int32_t>(gx + kHaloWidth),
                          static_cast<std::int32_t>((gy - m_y0) + kHaloWidth)};
}

void Mpi2SoaBackend::maybe_print_partition_debug()
{
    if (m_partition_logged || !m_partition_ready || !mpi2_debug_partition_enabled()) {
        return;
    }

    // Debug-only line per rank to validate row partition and neighbor assignment.
    std::cerr << "INFO mpi2 partition rank=" << m_rank << "/" << m_size
              << " W=" << m_global_w
              << " H=" << m_global_h
              << " y0=" << m_y0
              << " y1=" << m_y1
              << " rows=" << (m_y1 - m_y0)
              << " up=" << m_up_rank
              << " down=" << m_down_rank
              << '\n';

    m_partition_logged = true;
}
