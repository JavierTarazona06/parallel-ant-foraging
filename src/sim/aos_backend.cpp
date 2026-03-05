#include "aos_backend.hpp"

#include "../renderer.hpp"
#include "../timing_profile.hpp"

AosBackend::AosBackend(std::vector<ant>& ants) : m_ants(ants) {}

const char* AosBackend::name() const
{
    return "aos";
}

void AosBackend::step(WorldState& world, const SimConfig& sim_config)
{
    const std::uint64_t t0_ns = profile_now_ns();
    for (std::size_t i = 0; i < m_ants.size(); ++i) {
        m_ants[i].advance(world.phen, world.land, sim_config.pos_food, sim_config.pos_nest, world.food_quantity,
                          world.profile);
    }
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

std::unique_ptr<Renderer> AosBackend::create_renderer(const fractal_land& land,
                                                      const pheronome& phen,
                                                      const position_t& pos_nest,
                                                      const position_t& pos_food) const
{
    return std::make_unique<Renderer>(land, phen, pos_nest, pos_food, m_ants);
}
