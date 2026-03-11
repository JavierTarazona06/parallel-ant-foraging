#include "omp_soa_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <omp.h>

#include "../rand_generator.hpp"
#include "../renderer.hpp"
#include "../soa_ant_step.hpp"
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
            auto mark_sink = [&local, stride](std::int32_t mark_x, std::int32_t mark_y, int ind_pher) {
                const std::size_t touched_idx = flat_cell_index(mark_x, mark_y, stride);
                if (ind_pher == 0) {
                    local.touched_v1.push_back(touched_idx);
                } else {
                    local.touched_v2.push_back(touched_idx);
                }
            };
            advance_one_ant_soa_core(world.land, world.phen, sim_config.pos_nest.x, sim_config.pos_nest.y,
                                     sim_config.pos_food.x, sim_config.pos_food.y, m_ants, ant_index,
                                     sim_config.epsilon, local.food_delta, mark_sink, local.k2_ns, local.k3_ns);
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

    std::sort(merged_touched_v1.begin(), merged_touched_v1.end());
    merged_touched_v1.erase(std::unique(merged_touched_v1.begin(), merged_touched_v1.end()), merged_touched_v1.end());

    std::sort(merged_touched_v2.begin(), merged_touched_v2.end());
    merged_touched_v2.erase(std::unique(merged_touched_v2.begin(), merged_touched_v2.end()), merged_touched_v2.end());
    const std::size_t touched_raw_total = total_touched_v1 + total_touched_v2;
    const std::size_t touched_unique_total = merged_touched_v1.size() + merged_touched_v2.size();

    // Replay marks only once per unique touched cell per channel outside
    // the OpenMP region to avoid races on the pheromone map.
    for (const std::size_t idx : merged_touched_v1) {
        replay_mark_from_index(world.phen, idx, stride);
    }
    for (const std::size_t idx : merged_touched_v2) {
        replay_mark_from_index(world.phen, idx, stride);
    }

    world.food_quantity += merged_food_delta;

    const std::uint64_t t1_ns = profile_now_ns();

    world.phen.do_evaporation();
    const std::uint64_t t2_ns = profile_now_ns();

    world.phen.update();
    const std::uint64_t t3_ns = profile_now_ns();

    if (world.iter_timing != nullptr) {
        world.iter_timing->k1_ns += (t1_ns - t0_ns);
        world.iter_timing->k2_ns += merged_k2_ns;
        world.iter_timing->k3_ns += merged_k3_ns;
        world.iter_timing->k4_ns += (t2_ns - t1_ns);
        world.iter_timing->k5_ns += (t3_ns - t2_ns);
        world.iter_timing->touched_raw_count += touched_raw_total;
        world.iter_timing->touched_unique_count += touched_unique_total;
    }
}

std::unique_ptr<Renderer> OmpSoaBackend::create_renderer(const fractal_land& land,
                                                         const pheronome& phen,
                                                         const position_t& pos_nest,
                                                         const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
