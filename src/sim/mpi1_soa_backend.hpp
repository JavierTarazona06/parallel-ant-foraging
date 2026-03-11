#pragma once

#include "backend.hpp"

class Mpi1SoaBackend final : public Backend
{
public:
    explicit Mpi1SoaBackend(AntsSoA& ants, std::size_t mpi_sync_every);

    const char* name() const override;
    void step(WorldState& world, const SimConfig& sim_config) override;
    std::unique_ptr<Renderer> create_renderer(const fractal_land& land,
                                              const pheronome& phen,
                                              const position_t& pos_nest,
                                              const position_t& pos_food) const override;

private:
    AntsSoA& m_ants;
    std::size_t m_sync_every{1};
    std::size_t m_iteration{0};
};
