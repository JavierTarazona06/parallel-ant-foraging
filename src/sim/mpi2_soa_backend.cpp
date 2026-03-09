#include "mpi2_soa_backend.hpp"

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
    // MPI2 skeleton: intentionally no simulation logic yet.
    (void)world;
    (void)sim_config;
}

std::unique_ptr<Renderer> Mpi2SoaBackend::create_renderer(const fractal_land& land,
                                                          const pheronome& phen,
                                                          const position_t& pos_nest,
                                                          const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
