#pragma once

#include <cstddef>
#include <cstdint>

#include "../app/cli.hpp"

class fractal_land;
class pheronome;
class Renderer;
class Window;
class Backend;

struct MeasurementTotals {
    std::uint64_t p0_ns{0};
    std::uint64_t p1_ns{0};
    std::uint64_t p2_ns{0};
    std::uint64_t k0_ns{0};
    std::uint64_t k1_ns{0};
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
    std::uint64_t r0_ns{0};
    std::uint64_t e0_ns{0};
    std::size_t measured_iterations{0};
};

void run_benchmark(const RunConfig& run_config,
                   const SimConfig& sim_config,
                   const fractal_land& land,
                   pheronome& phen,
                   Backend& backend,
                   bool render_enabled,
                   Renderer* renderer,
                   Window* win,
                   std::size_t& food_quantity,
                   MeasurementTotals& totals);

void run_interactive(const RunConfig& run_config,
                     const SimConfig& sim_config,
                     const fractal_land& land,
                     pheronome& phen,
                     Backend& backend,
                     Renderer* renderer,
                     Window* win,
                     std::size_t& food_quantity);
