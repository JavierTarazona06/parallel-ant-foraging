#pragma once

#include "backend.hpp"

class Mpi2SoaBackend : public Backend
{
public:
    explicit Mpi2SoaBackend(AntsSoA& ants);

    const char* name() const override;
    void step(WorldState& world, const SimConfig& sim_config) override;
    std::unique_ptr<Renderer> create_renderer(const fractal_land& land,
                                              const pheronome& phen,
                                              const position_t& pos_nest,
                                              const position_t& pos_food) const override;

private:
    AntsSoA& m_ants;
};

