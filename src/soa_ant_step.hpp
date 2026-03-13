#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "ants_soa.hpp"
#include "rand_generator.hpp"
#include "timing_profile.hpp"

template<typename MarkSink>
inline void advance_one_ant_soa_core(const fractal_land& land,
                                     const pheronome& phen_read,
                                     std::int32_t nest_x,
                                     std::int32_t nest_y,
                                     std::int32_t food_x,
                                     std::int32_t food_y,
                                     AntsSoA& ants,
                                     std::size_t ant_index,
                                     double eps,
                                     std::size_t& food_counter,
                                     MarkSink&& mark_sink,
                                     std::uint64_t& k2_work_ns,
                                     std::uint64_t& k3_work_ns)
{
    // Copy the ant state locally so the inner loop writes back only once per iteration.
    std::uint32_t seed = ants.seed[ant_index];
    std::uint8_t state = ants.state[ant_index];
    std::int32_t x = ants.x[ant_index];
    std::int32_t y = ants.y[ant_index];
    double consumed_time = 0.;

    // Keep moving until the ant has consumed the full iteration time budget.
    while (consumed_time < 1.) {
        const std::uint64_t ant_step_start_ns = profile_now_ns();

        // Select the pheromone channel that matches the current loaded or unloaded state.
        const int ind_pher = (state == 1u) ? 1 : 0;
        const double choix = rand_double(0., 1., seed);
        std::int32_t new_x = x;
        std::int32_t new_y = y;

        // Inspect the four-neighbor pheromone field before choosing the next cell.
        const double max_phen =
            std::max({phen_read(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher],
                      phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y + 1))[ind_pher]});

        // Explore randomly when epsilon wins or no positive pheromone signal is available.
        if ((choix > eps) || (max_phen <= 0.)) {
            do {
                new_x = x;
                new_y = y;
                const int d = rand_int32(1, 4, seed);
                if (d == 1) {
                    new_x -= 1;
                }
                if (d == 2) {
                    new_y -= 1;
                }
                if (d == 3) {
                    new_x += 1;
                }
                if (d == 4) {
                    new_y += 1;
                }
            } while (phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y))[ind_pher] == -1);
        } else {
            // Follow the strongest neighboring pheromone when the greedy branch is selected.
            if (phen_read(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher] == max_phen)
            {
                new_x -= 1;
            } else if (phen_read(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher] ==
                       max_phen) {
                new_x += 1;
            } else if (phen_read(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher] ==
                       max_phen) {
                new_y -= 1;
            } else {
                new_y += 1;
            }
        }

        // Charge the terrain cost of the chosen cell against the current iteration budget.
        consumed_time += land(static_cast<unsigned long>(new_x), static_cast<unsigned long>(new_y));
        k2_work_ns += (profile_now_ns() - ant_step_start_ns);

        // Delegate the pheromone write so each backend can choose its own mark strategy.
        const std::uint64_t mark_start_ns = profile_now_ns();
        mark_sink(new_x, new_y, ind_pher);
        k3_work_ns += (profile_now_ns() - mark_start_ns);

        // Update the ant state and handle nest or food transitions after the move is accepted.
        const std::uint64_t k2_tail_start_ns = profile_now_ns();
        x = new_x;
        y = new_y;

        if ((x == nest_x) && (y == nest_y)) {
            if (state == 1u) {
                food_counter += 1;
            }
            state = 0u;
        }
        if ((x == food_x) && (y == food_y)) {
            state = 1u;
        }
        k2_work_ns += (profile_now_ns() - k2_tail_start_ns);
    }

    // Write the final local state back into the SoA arrays once the iteration is complete.
    ants.x[ant_index] = x;
    ants.y[ant_index] = y;
    ants.state[ant_index] = state;
    ants.seed[ant_index] = seed;
}
