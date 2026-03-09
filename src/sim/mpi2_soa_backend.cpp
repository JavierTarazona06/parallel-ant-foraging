#include "mpi2_soa_backend.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

#include <mpi.h>

#include "../mpi/mpi_runtime.hpp"
#include "../renderer.hpp"

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
