#pragma once

#include "backend.hpp"

class AosBackend final : public Backend
{
public:
    // Wrap the original AoS simulation loop behind the shared backend interface.
    explicit AosBackend(std::vector<ant>& ants);

    const char* name() const override;
    void step(WorldState& world, const SimConfig& sim_config) override;
    std::unique_ptr<Renderer> create_renderer(const fractal_land& land,
                                              const pheronome& phen,
                                              const position_t& pos_nest,
                                              const position_t& pos_food) const override;

private:
    std::vector<ant>& m_ants;
};
