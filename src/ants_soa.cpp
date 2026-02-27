#include "ants_soa.hpp"

#include <algorithm>

#include "rand_generator.hpp"
#include "timing_profile.hpp"

namespace {

inline void advance_one_ant_soa(const fractal_land& land, pheronome& phen,
                                std::int32_t nest_x, std::int32_t nest_y,
                                std::int32_t food_x, std::int32_t food_y,
                                AntsSoA& ants, std::size_t ant_index, double eps,
                                std::size_t& food_counter)
{
    std::uint32_t& seed = ants.seed[ant_index];
    std::uint8_t& state = ants.state[ant_index];
    std::int32_t x = ants.x[ant_index];
    std::int32_t y = ants.y[ant_index];
    double consumed_time = 0.;

    while (consumed_time < 1.) {
        const std::uint64_t ant_step_start_ns = profile_now_ns();

        const int ind_pher = (state == 1u) ? 1 : 0;
        const double choix = rand_double(0., 1., seed);
        std::int32_t new_x = x;
        std::int32_t new_y = y;

        const double max_phen = std::max(
            {phen(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher],
             phen(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher],
             phen(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher],
             phen(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y + 1))[ind_pher]});

        if ((choix > eps) || (max_phen <= 0.)) {
            do {
                new_x = x;
                new_y = y;
                const int d = rand_int32(1, 4, seed);
                if (d == 1)
                    new_x -= 1;
                if (d == 2)
                    new_y -= 1;
                if (d == 3)
                    new_x += 1;
                if (d == 4)
                    new_y += 1;
            } while (phen(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y))[ind_pher] == -1);
        } else {
            if (phen(static_cast<std::size_t>(new_x - 1), static_cast<std::size_t>(new_y))[ind_pher] == max_phen)
                new_x -= 1;
            else if (phen(static_cast<std::size_t>(new_x + 1), static_cast<std::size_t>(new_y))[ind_pher] ==
                     max_phen)
                new_x += 1;
            else if (phen(static_cast<std::size_t>(new_x), static_cast<std::size_t>(new_y - 1))[ind_pher] ==
                     max_phen)
                new_y -= 1;
            else
                new_y += 1;
        }

        consumed_time += land(static_cast<unsigned long>(new_x), static_cast<unsigned long>(new_y));
        const std::uint64_t before_mark_ns = profile_now_ns();
        profile_add_k2(before_mark_ns - ant_step_start_ns);

        phen.mark_pheronome(new_x, new_y);
        const std::uint64_t after_mark_ns = profile_now_ns();
        profile_add_k3(after_mark_ns - before_mark_ns);

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

        const std::uint64_t ant_step_end_ns = profile_now_ns();
        profile_add_k2(ant_step_end_ns - after_mark_ns);
    }

    ants.x[ant_index] = x;
    ants.y[ant_index] = y;
}

} // namespace

void advance_time_soa(const fractal_land& land, pheronome& phen,
                      std::int32_t nest_x, std::int32_t nest_y,
                      std::int32_t food_x, std::int32_t food_y,
                      AntsSoA& ants, double eps, std::size_t& food_counter,
                      IterTimingNs* iter_timing)
{
    const std::uint64_t t0_ns = profile_now_ns();
    for (std::size_t i = 0; i < ants.size(); ++i) {
        advance_one_ant_soa(land, phen, nest_x, nest_y, food_x, food_y, ants, i, eps, food_counter);
    }
    const std::uint64_t t1_ns = profile_now_ns();

    phen.do_evaporation();
    const std::uint64_t t2_ns = profile_now_ns();

    phen.update();
    const std::uint64_t t3_ns = profile_now_ns();

    if (iter_timing != nullptr) {
        iter_timing->k1_ns += (t1_ns - t0_ns);
        iter_timing->k4_ns += (t2_ns - t1_ns);
        iter_timing->k5_ns += (t3_ns - t2_ns);
    }
}

