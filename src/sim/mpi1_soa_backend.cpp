#include "mpi1_soa_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "../mpi/mpi_runtime.hpp"
#include "../renderer.hpp"
#include "../timing_profile.hpp"

Mpi1SoaBackend::Mpi1SoaBackend(AntsSoA& ants) : m_ants(ants) {}

const char* Mpi1SoaBackend::name() const
{
    return "mpi1+soa";
}

void Mpi1SoaBackend::step(WorldState& world, const SimConfig& sim_config)
{
    const std::size_t ant_count = m_ants.size();
    const std::size_t mpi_size = static_cast<std::size_t>(std::max(1, mpi_runtime::size()));
    const std::size_t mpi_rank = static_cast<std::size_t>(std::max(0, mpi_runtime::rank()));

    const std::size_t base = ant_count / mpi_size;
    const std::size_t remainder = ant_count % mpi_size;
    const std::size_t begin = mpi_rank * base + std::min(mpi_rank, remainder);
    const std::size_t end = begin + base + (mpi_rank < remainder ? 1u : 0u);
    std::size_t food_delta_local = 0;

    // MPI1 phase 1: each rank only advances its local ant block.
    advance_ants_soa_range(world.land, world.phen, sim_config.pos_nest.x, sim_config.pos_nest.y, sim_config.pos_food.x,
                           sim_config.pos_food.y, m_ants, begin, end, sim_config.epsilon, food_delta_local,
                           world.profile, world.iter_timing);

    const std::uint64_t food_delta_global_u64 =
        mpi_runtime::allreduce_sum_uint64(static_cast<std::uint64_t>(food_delta_local));
    world.food_quantity += static_cast<std::size_t>(food_delta_global_u64);

    const std::uint64_t evap_start_ns = profile_now_ns();
    world.phen.do_evaporation();
    const std::uint64_t evap_end_ns = profile_now_ns();

    world.phen.update();
    const std::uint64_t update_end_ns = profile_now_ns();

    mpi_runtime::allreduce_max_double_array(world.phen.v1_data(), world.phen.v1_size());
    mpi_runtime::allreduce_max_double_array(world.phen.v2_data(), world.phen.v2_size());

    if (world.iter_timing != nullptr) {
        world.iter_timing->k4_ns += (evap_end_ns - evap_start_ns);
        world.iter_timing->k5_ns += (update_end_ns - evap_end_ns);
    }
}

std::unique_ptr<Renderer> Mpi1SoaBackend::create_renderer(const fractal_land& land,
                                                          const pheronome& phen,
                                                          const position_t& pos_nest,
                                                          const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
