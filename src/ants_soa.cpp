#include "ants_soa.hpp"
#include "soa_ant_step.hpp"
#include "timing_profile.hpp"

void advance_time_soa(const fractal_land& land, pheronome& phen,
                      std::int32_t nest_x, std::int32_t nest_y,
                      std::int32_t food_x, std::int32_t food_y,
                      AntsSoA& ants, double eps, std::size_t& food_counter, TimingProfile& profile,
                      IterTimingNs* iter_timing)
{
    (void)profile;
    const std::uint64_t t0_ns = profile_now_ns();
    std::uint64_t k2_work_ns = 0;
    std::uint64_t k3_work_ns = 0;
    const pheronome& phen_read = phen;
    auto mark_sink = [&phen](std::int32_t mark_x, std::int32_t mark_y, int /*ind_pher*/) {
        phen.mark_pheronome(mark_x, mark_y);
    };
    for (std::size_t i = 0; i < ants.size(); ++i) {
        advance_one_ant_soa_core(land, phen_read, nest_x, nest_y, food_x, food_y, ants, i, eps, food_counter,
                                 mark_sink, k2_work_ns, k3_work_ns);
    }
    const std::uint64_t t1_ns = profile_now_ns();

    phen.do_evaporation();
    const std::uint64_t t2_ns = profile_now_ns();

    phen.update();
    const std::uint64_t t3_ns = profile_now_ns();

    if (iter_timing != nullptr) {
        iter_timing->k1_ns += (t1_ns - t0_ns);
        iter_timing->k2_ns += k2_work_ns;
        iter_timing->k3_ns += k3_work_ns;
        iter_timing->k4_ns += (t2_ns - t1_ns);
        iter_timing->k5_ns += (t3_ns - t2_ns);
    }
}
