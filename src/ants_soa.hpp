#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "fractal_land.hpp"
#include "pheronome.hpp"

class TimingProfile;

struct IterTimingNs {
    // Store per-iteration kernel timings and MPI extras for benchmark reporting.
    std::uint64_t k1_ns{0};
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
    std::uint64_t k_mpi_sync_ns{0};
    std::uint64_t k_mpi_halo_ns{0};
    std::uint64_t k_mpi_migrate_ns{0};
    std::uint64_t touched_raw_count{0};
    std::uint64_t touched_unique_count{0};
};

struct AntsSoA {
    // Store ant attributes in separate arrays to favor vector-friendly traversal.
    std::vector<std::int32_t> x;
    std::vector<std::int32_t> y;
    std::vector<std::uint8_t> state;
    std::vector<std::uint32_t> seed;

    void reserve(std::size_t n)
    {
        x.reserve(n);
        y.reserve(n);
        state.reserve(n);
        seed.reserve(n);
    }

    std::size_t size() const { return x.size(); }

    void push_back(std::int32_t ant_x, std::int32_t ant_y, std::uint32_t ant_seed, std::uint8_t ant_state)
    {
        x.push_back(ant_x);
        y.push_back(ant_y);
        state.push_back(ant_state);
        seed.push_back(ant_seed);
    }
};

// Run a full serial SoA iteration on the whole ant population.
void advance_time_soa(const fractal_land& land, pheronome& phen,
                      std::int32_t nest_x, std::int32_t nest_y,
                      std::int32_t food_x, std::int32_t food_y,
                      AntsSoA& ants, double eps, std::size_t& food_counter,
                      TimingProfile& profile,
                      IterTimingNs* iter_timing = nullptr);

// Run a full serial SoA iteration on a subrange of the ant population.
void advance_time_soa_range(const fractal_land& land, pheronome& phen,
                            std::int32_t nest_x, std::int32_t nest_y,
                            std::int32_t food_x, std::int32_t food_y,
                            AntsSoA& ants, std::size_t begin, std::size_t end,
                            double eps, std::size_t& food_counter,
                            TimingProfile& profile,
                            IterTimingNs* iter_timing = nullptr);

// Advance only the ant-move phase on a subrange before evaporation and update.
void advance_ants_soa_range(const fractal_land& land, pheronome& phen,
                            std::int32_t nest_x, std::int32_t nest_y,
                            std::int32_t food_x, std::int32_t food_y,
                            AntsSoA& ants, std::size_t begin, std::size_t end,
                            double eps, std::size_t& food_counter,
                            TimingProfile& profile,
                            IterTimingNs* iter_timing = nullptr);
