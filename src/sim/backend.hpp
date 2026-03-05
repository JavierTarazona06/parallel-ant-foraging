#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "../ant.hpp"
#include "../ants_soa.hpp"
#include "../app/cli.hpp"

class Renderer;
class fractal_land;
class pheronome;
class TimingProfile;

struct WorldState {
    const fractal_land& land;
    pheronome& phen;
    std::size_t& food_quantity;
    TimingProfile& profile;
    IterTimingNs* iter_timing{nullptr};
};

class Backend
{
public:
    virtual ~Backend() = default;
    virtual const char* name() const = 0;
    virtual void step(WorldState& world, const SimConfig& sim_config) = 0;
    virtual std::unique_ptr<Renderer> create_renderer(const fractal_land& land,
                                                      const pheronome& phen,
                                                      const position_t& pos_nest,
                                                      const position_t& pos_food) const = 0;
};

std::unique_ptr<Backend> make_backend(const RunConfig& run_config,
                                      const SimConfig& sim_config,
                                      const WorldState& world_state,
                                      std::vector<ant>& ants_aos,
                                      AntsSoA& ants_soa);
