#include "mpi2_soa_backend.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#include <mpi.h>

#include "../mpi/mpi_runtime.hpp"
#include "../rand_generator.hpp"
#include "../renderer.hpp"
#include "../timing_profile.hpp"

namespace {

constexpr std::size_t kHaloWidth = 1u;

// Baseline MPI2 migration policy (deferred migration):
// In this project, an ant can do multiple substeps per iteration (consumed_time < 1.0).
// For the baseline mpi2 path, when an ant crosses a domain boundary, it is transferred
// to the neighbor rank but does not continue its remaining substeps in the same iteration.
// It continues on the destination rank at the next iteration.
// This is a documented approximation for course-level bonus scope.

bool mpi2_debug_partition_enabled()
{
    const char* env = std::getenv("MPI2_DEBUG_PARTITION");
    if (env == nullptr) {
        return false;
    }
    const std::string value(env);
    return value == "1" || value == "true" || value == "TRUE" || value == "yes" || value == "YES";
}

bool mpi2_debug_migration_enabled()
{
    const char* env = std::getenv("MPI2_DEBUG_MIGRATION");
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
    // MPI2 path: partition/halo + local-ant step + deferred migration outboxes.
    initialize_partition_if_needed(world);
    initialize_local_ants_if_needed();

    const std::uint64_t halo_start_ns = profile_now_ns();
    halo_exchange();
    const std::uint64_t halo_end_ns = profile_now_ns();

    if (world.iter_timing != nullptr) {
        world.iter_timing->k_mpi_halo_ns += (halo_end_ns - halo_start_ns);
    }

    step_local_ants(world, sim_config);
    maybe_print_migration_debug();
    ++m_step_counter;
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

void Mpi2SoaBackend::initialize_local_ants_if_needed()
{
    if (!m_partition_ready || m_local_ants_ready) {
        return;
    }

    m_local_ants.clear();
    m_local_ants.reserve(m_ants.size());

    for (std::size_t i = 0; i < m_ants.size(); ++i) {
        const std::int32_t ax = m_ants.x[i];
        const std::int32_t ay = m_ants.y[i];
        if (owns_cell_global(ax, ay)) {
            m_local_ants.push_back(ax, ay, m_ants.state[i], m_ants.seed[i]);
        }
    }

    m_local_ants_ready = true;
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

void Mpi2SoaBackend::step_local_ants(WorldState& world, const SimConfig& sim_config)
{
    if (!m_local_ants_ready || m_local_ants.empty()) {
        m_outbox_up.clear();
        m_outbox_down.clear();
        return;
    }

    m_outbox_up.clear();
    m_outbox_down.clear();
    m_outbox_up.reserve(m_local_ants.size() / 4 + 4);
    m_outbox_down.reserve(m_local_ants.size() / 4 + 4);

    const std::uint64_t step_start_ns = profile_now_ns();
    std::uint64_t k2_work_ns = 0;
    std::uint64_t k3_work_ns = 0;

    std::size_t write_idx = 0;
    const std::size_t initial_count = m_local_ants.size();
    for (std::size_t i = 0; i < initial_count; ++i) {
        const bool migrated = advance_one_local_ant(world, sim_config, i);
        if (migrated) {
            enqueue_migrated_ant(i);
            continue;
        }

        if (write_idx != i) {
            m_local_ants.x[write_idx] = m_local_ants.x[i];
            m_local_ants.y[write_idx] = m_local_ants.y[i];
            m_local_ants.state[write_idx] = m_local_ants.state[i];
            m_local_ants.seed[write_idx] = m_local_ants.seed[i];
        }
        ++write_idx;
    }
    m_local_ants.resize(write_idx);
    const std::uint64_t step_end_ns = profile_now_ns();

    if (world.iter_timing != nullptr) {
        world.iter_timing->k1_ns += (step_end_ns - step_start_ns);
        world.iter_timing->k2_ns += k2_work_ns;
        world.iter_timing->k3_ns += k3_work_ns;
    }
}

bool Mpi2SoaBackend::advance_one_local_ant(WorldState& world, const SimConfig& sim_config, std::size_t ant_idx)
{
    std::uint32_t seed = m_local_ants.seed[ant_idx];
    std::uint8_t state = m_local_ants.state[ant_idx];
    std::int32_t x = m_local_ants.x[ant_idx];
    std::int32_t y = m_local_ants.y[ant_idx];
    double consumed_time = 0.0;

    while (consumed_time < 1.0) {
        const int ind_pher = (state == 1u) ? 1 : 0;
        const double choix = rand_double(0., 1., seed);
        std::int32_t new_x = x;
        std::int32_t new_y = y;

        const double max_phen =
            std::max({phen_read_global(new_x - 1, new_y, ind_pher),
                      phen_read_global(new_x + 1, new_y, ind_pher),
                      phen_read_global(new_x, new_y - 1, ind_pher),
                      phen_read_global(new_x, new_y + 1, ind_pher)});

        if ((choix > sim_config.epsilon) || (max_phen <= 0.)) {
            do {
                new_x = x;
                new_y = y;
                const int d = rand_int32(1, 4, seed);
                if (d == 1) {
                    new_x -= 1;
                }
                if (d == 2) {
                    new_y -= 1;
                }
                if (d == 3) {
                    new_x += 1;
                }
                if (d == 4) {
                    new_y += 1;
                }
            } while (phen_read_global(new_x, new_y, ind_pher) == -1.0);
        } else {
            if (phen_read_global(new_x - 1, new_y, ind_pher) == max_phen) {
                new_x -= 1;
            } else if (phen_read_global(new_x + 1, new_y, ind_pher) == max_phen) {
                new_x += 1;
            } else if (phen_read_global(new_x, new_y - 1, ind_pher) == max_phen) {
                new_y -= 1;
            } else {
                new_y += 1;
            }
        }

        // Deferred migration baseline: ant is moved to neighbor outbox and stops this iteration.
        if (!owns_cell_global(new_x, new_y)) {
            m_local_ants.x[ant_idx] = new_x;
            m_local_ants.y[ant_idx] = new_y;
            m_local_ants.state[ant_idx] = state;
            m_local_ants.seed[ant_idx] = seed;
            return true;
        }

        consumed_time += world.land(static_cast<unsigned long>(new_x), static_cast<unsigned long>(new_y));
        (void)mark_pheromone_global(new_x, new_y, ind_pher);
        x = new_x;
        y = new_y;

        if ((x == sim_config.pos_nest.x) && (y == sim_config.pos_nest.y)) {
            if (state == 1u) {
                world.food_quantity += 1;
            }
            state = 0u;
        }
        if ((x == sim_config.pos_food.x) && (y == sim_config.pos_food.y)) {
            state = 1u;
        }
    }

    m_local_ants.x[ant_idx] = x;
    m_local_ants.y[ant_idx] = y;
    m_local_ants.state[ant_idx] = state;
    m_local_ants.seed[ant_idx] = seed;
    return false;
}

void Mpi2SoaBackend::enqueue_migrated_ant(std::size_t ant_idx)
{
    const std::int32_t ax = m_local_ants.x[ant_idx];
    const std::int32_t ay = m_local_ants.y[ant_idx];
    const std::uint8_t as = m_local_ants.state[ant_idx];
    const std::uint32_t sd = m_local_ants.seed[ant_idx];

    if (ay < static_cast<std::int32_t>(m_y0)) {
        if (m_up_rank != MPI_PROC_NULL) {
            m_outbox_up.push_back(ax, ay, as, sd);
        }
        return;
    }
    if (ay >= static_cast<std::int32_t>(m_y1)) {
        if (m_down_rank != MPI_PROC_NULL) {
            m_outbox_down.push_back(ax, ay, as, sd);
        }
        return;
    }

    // 1D row split owns full X span; unexpected migration on X is dropped for baseline.
}

void Mpi2SoaBackend::maybe_print_migration_debug()
{
    if (!mpi2_debug_migration_enabled()) {
        return;
    }

    std::cerr << "INFO mpi2 migration rank=" << m_rank
              << " step=" << m_step_counter
              << " local_ants=" << m_local_ants.size()
              << " out_up=" << m_outbox_up.size()
              << " out_down=" << m_outbox_down.size()
              << '\n';
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
