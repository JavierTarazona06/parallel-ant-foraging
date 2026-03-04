#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "fractal_land.hpp"
#include "pheronome.hpp"

class TimingProfile;

struct IterTimingNs {
    std::uint64_t k1_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
};

struct AntsSoA {
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

void advance_time_soa(const fractal_land& land, pheronome& phen,
                      std::int32_t nest_x, std::int32_t nest_y,
                      std::int32_t food_x, std::int32_t food_y,
                      AntsSoA& ants, double eps, std::size_t& food_counter,
                      TimingProfile& profile,
                      IterTimingNs* iter_timing = nullptr);
