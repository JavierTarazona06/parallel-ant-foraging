#include "mpi2_soa_backend.hpp"

#include "../ants_soa.hpp"
#include "../renderer.hpp"

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
    // MPI2 skeleton: keep SoA serial step semantics for now.
    advance_time_soa(world.land, world.phen, sim_config.pos_nest.x, sim_config.pos_nest.y, sim_config.pos_food.x,
                     sim_config.pos_food.y, m_ants, sim_config.epsilon, world.food_quantity, world.profile,
                     world.iter_timing);
}

std::unique_ptr<Renderer> Mpi2SoaBackend::create_renderer(const fractal_land& land,
                                                          const pheronome& phen,
                                                          const position_t& pos_nest,
                                                          const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}

