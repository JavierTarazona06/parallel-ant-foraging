#include "omp_soa_backend.hpp"

#include "../renderer.hpp"

OmpSoaBackend::OmpSoaBackend(AntsSoA& ants) : m_ants(ants) {}

const char* OmpSoaBackend::name() const
{
    return "omp+soa";
}

void OmpSoaBackend::step(WorldState& world, const SimConfig& sim_config)
{
    // OpenMP-safe kernel currently enabled only in evaporation.
    advance_time_soa(world.land, world.phen, sim_config.pos_nest.x, sim_config.pos_nest.y, sim_config.pos_food.x,
                     sim_config.pos_food.y, m_ants, sim_config.epsilon, world.food_quantity, world.profile,
                     world.iter_timing);
}

std::unique_ptr<Renderer> OmpSoaBackend::create_renderer(const fractal_land& land,
                                                         const pheronome& phen,
                                                         const position_t& pos_nest,
                                                         const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
