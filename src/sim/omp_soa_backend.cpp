#include "omp_soa_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <omp.h>

#include "../rand_generator.hpp"
#include "../renderer.hpp"
#include "../timing_profile.hpp"

namespace {

struct ThreadTouchedCells {
    std::vector<std::size_t> touched_v1;
    std::vector<std::size_t> touched_v2;
    std::size_t food_delta{0};
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
};

inline std::size_t flat_cell_index(std::int32_t x, std::int32_t y, std::size_t stride)
{
    return (static_cast<std::size_t>(x) + 1u) * stride + (static_cast<std::size_t>(y) + 1u);
}

inline void replay_mark_from_index(pheronome& phen, std::size_t idx, std::size_t stride)
{
    const std::size_t x = idx / stride - 1u;
    const std::size_t y = idx % stride - 1u;
    phen.mark_pheronome(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y));
}

inline void advance_one_ant_soa_omp(const fractal_land& land,
                                    const pheronome& phen_read,
                                    std::int32_t nest_x,
                                    std::int32_t nest_y,
                                    std::int32_t food_x,
                                    std::int32_t food_y,
                                    double eps,
                                    std::size_t stride,
                                    AntsSoA& ants,
                                    std::size_t ant_index,
                                    ThreadTouchedCells& local)
{
    std::uint32_t seed = ants.seed[ant_index];
    std::uint8_t state = ants.state[ant_index];
    std::int32_t x = ants.x[ant_index];
    std::int32_t y = ants.y[ant_index];
    double consumed_time = 0.;

    while (consumed_time < 1.) {
        const std::uint64_t ant_step_start_ns = profile_now_ns();

        const int ind_pher = (state == 1u) ? 1 : 0;
        const double choix = rand_double(0., 1., seed);
        std::int32_t new_x = x;
        std::int32_t new_y = y;

        const double max_phen =
            std::max({phen_read(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y + 1))[ind_pher]});

        if ((choix > eps) || (max_phen <= 0.)) {
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
            } while (phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y))[ind_pher] == -1);
        } else {
            if (phen_read(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher] == max_phen)
            {
                new_x -= 1;
            } else if (phen_read(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher] ==
                       max_phen) {
                new_x += 1;
            } else if (phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher] ==
                       max_phen) {
                new_y -= 1;
            } else {
                new_y += 1;
            }
        }

        consumed_time += land(static_cast<unsigned long>(new_x), static_cast<unsigned long>(new_y));
        local.k2_ns += (profile_now_ns() - ant_step_start_ns);

        const std::uint64_t mark_start_ns = profile_now_ns();
        const std::size_t touched_idx = flat_cell_index(new_x, new_y, stride);
        if (ind_pher == 0) {
            local.touched_v1.push_back(touched_idx);
        } else {
            local.touched_v2.push_back(touched_idx);
        }
        local.k3_ns += (profile_now_ns() - mark_start_ns);

        const std::uint64_t k2_tail_start_ns = profile_now_ns();
        x = new_x;
        y = new_y;

        if ((x == nest_x) && (y == nest_y)) {
            if (state == 1u) {
                local.food_delta += 1;
            }
            state = 0u;
        }
        if ((x == food_x) && (y == food_y)) {
            state = 1u;
        }

        local.k2_ns += (profile_now_ns() - k2_tail_start_ns);
    }

    ants.x[ant_index] = x;
    ants.y[ant_index] = y;
    ants.state[ant_index] = state;
    ants.seed[ant_index] = seed;
}

} // namespace

OmpSoaBackend::OmpSoaBackend(AntsSoA& ants) : m_ants(ants) {}

const char* OmpSoaBackend::name() const
{
    return "omp+soa";
}

void OmpSoaBackend::step(WorldState& world, const SimConfig& sim_config)
{
    const std::uint64_t t0_ns = profile_now_ns();
    const std::size_t stride = static_cast<std::size_t>(world.land.dimensions()) + 2u;
    const int max_threads = omp_get_max_threads();

    std::vector<ThreadTouchedCells> per_thread(static_cast<std::size_t>(max_threads));

#pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        ThreadTouchedCells& local = per_thread[static_cast<std::size_t>(tid)];
        local.touched_v1.clear();
        local.touched_v2.clear();
        local.food_delta = 0;
        local.k2_ns = 0;
        local.k3_ns = 0;

        const std::size_t chunk = (m_ants.size() + static_cast<std::size_t>(max_threads) - 1u) /
                                  static_cast<std::size_t>(max_threads);
        local.touched_v1.reserve(chunk);
        local.touched_v2.reserve(chunk);

#pragma omp for schedule(static)
        for (std::size_t ant_index = 0; ant_index < m_ants.size(); ++ant_index) {
            advance_one_ant_soa_omp(world.land, world.phen, sim_config.pos_nest.x, sim_config.pos_nest.y,
                                    sim_config.pos_food.x, sim_config.pos_food.y, sim_config.epsilon, stride, m_ants,
                                    ant_index, local);
        }
    }

    std::size_t merged_food_delta = 0;
    std::uint64_t merged_k2_ns = 0;
    std::uint64_t merged_k3_ns = 0;
    std::size_t total_touched_v1 = 0;
    std::size_t total_touched_v2 = 0;
    for (const ThreadTouchedCells& local : per_thread) {
        merged_food_delta += local.food_delta;
        merged_k2_ns += local.k2_ns;
        merged_k3_ns += local.k3_ns;
        total_touched_v1 += local.touched_v1.size();
        total_touched_v2 += local.touched_v2.size();
    }

    std::vector<std::size_t> merged_touched_v1;
    std::vector<std::size_t> merged_touched_v2;
    merged_touched_v1.reserve(total_touched_v1);
    merged_touched_v2.reserve(total_touched_v2);

    for (const ThreadTouchedCells& local : per_thread) {
        merged_touched_v1.insert(merged_touched_v1.end(), local.touched_v1.begin(), local.touched_v1.end());
        merged_touched_v2.insert(merged_touched_v2.end(), local.touched_v2.begin(), local.touched_v2.end());
    }

    const std::uint64_t mark_merge_start_ns = profile_now_ns();
    std::sort(merged_touched_v1.begin(), merged_touched_v1.end());
    merged_touched_v1.erase(std::unique(merged_touched_v1.begin(), merged_touched_v1.end()), merged_touched_v1.end());

    std::sort(merged_touched_v2.begin(), merged_touched_v2.end());
    merged_touched_v2.erase(std::unique(merged_touched_v2.begin(), merged_touched_v2.end()), merged_touched_v2.end());

    // Replay marks only once per unique touched cell per channel outside
    // the OpenMP region to avoid races on the pheromone map.
    for (const std::size_t idx : merged_touched_v1) {
        replay_mark_from_index(world.phen, idx, stride);
    }
    for (const std::size_t idx : merged_touched_v2) {
        replay_mark_from_index(world.phen, idx, stride);
    }
    merged_k3_ns += (profile_now_ns() - mark_merge_start_ns);

    world.food_quantity += merged_food_delta;
    world.profile.add(TimingSection::k2, merged_k2_ns);
    world.profile.add(TimingSection::k3, merged_k3_ns);

    const std::uint64_t t1_ns = profile_now_ns();

    world.phen.do_evaporation();
    const std::uint64_t t2_ns = profile_now_ns();

    world.phen.update();
    const std::uint64_t t3_ns = profile_now_ns();

    if (world.iter_timing != nullptr) {
        world.iter_timing->k1_ns += (t1_ns - t0_ns);
        world.iter_timing->k4_ns += (t2_ns - t1_ns);
        world.iter_timing->k5_ns += (t3_ns - t2_ns);
    }
}

std::unique_ptr<Renderer> OmpSoaBackend::create_renderer(const fractal_land& land,
                                                         const pheronome& phen,
                                                         const position_t& pos_nest,
                                                         const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
