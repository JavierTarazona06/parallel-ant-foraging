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
    // Collect benchmark totals that are later emitted as METRIC lines.
    std::uint64_t p0_ns{0};
    std::uint64_t p1_ns{0};
    std::uint64_t p2_ns{0};
    std::uint64_t k0_ns{0};
    std::uint64_t k1_ns{0};
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
    std::uint64_t k_mpi_sync_ns{0};
    std::uint64_t k_mpi_halo_ns{0};
    std::uint64_t k_mpi_migrate_ns{0};
    std::uint64_t r0_ns{0};
    std::uint64_t e0_ns{0};
    std::size_t measured_iterations{0};
    std::uint64_t touched_raw_total{0};
    std::uint64_t touched_unique_total{0};
    std::size_t food_quantity_final{0};
    bool food_monotonic_ok{true};
    double phen_v1_sum{0.0};
    double phen_v2_sum{0.0};
    double phen_v1_max{0.0};
    double phen_v2_max{0.0};
};

// Execute the warmup and measured benchmark loop around the selected backend.
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

// Execute the interactive loop until the user closes the window.
void run_interactive(const RunConfig& run_config,
                     const SimConfig& sim_config,
                     const fractal_land& land,
                     pheronome& phen,
                     Backend& backend,
                     Renderer* renderer,
                     Window* win,
                     std::size_t& food_quantity);
