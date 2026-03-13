#include "ants_soa.hpp"
#include "soa_ant_step.hpp"
#include "timing_profile.hpp"

void advance_time_soa(const fractal_land& land, pheronome& phen,
                      std::int32_t nest_x, std::int32_t nest_y,
                      std::int32_t food_x, std::int32_t food_y,
                      AntsSoA& ants, double eps, std::size_t& food_counter, TimingProfile& profile,
                      IterTimingNs* iter_timing)
{
    // Run the full SoA iteration on the whole ant array through the range-based helper.
    advance_time_soa_range(land, phen, nest_x, nest_y, food_x, food_y, ants, 0, ants.size(), eps, food_counter,
                           profile, iter_timing);
}

void advance_time_soa_range(const fractal_land& land, pheronome& phen,
                            std::int32_t nest_x, std::int32_t nest_y,
                            std::int32_t food_x, std::int32_t food_y,
                            AntsSoA& ants, std::size_t begin, std::size_t end,
                            double eps, std::size_t& food_counter, TimingProfile& profile,
                            IterTimingNs* iter_timing)
{
    // Run the SoA ant-move phase on the requested range before map-wide updates.
    advance_ants_soa_range(land, phen, nest_x, nest_y, food_x, food_y, ants, begin, end, eps, food_counter, profile,
                           iter_timing);

    const std::uint64_t t1_ns = profile_now_ns();
    // Evaporate pheromones after all ants in the selected range have moved.
    phen.do_evaporation();
    const std::uint64_t t2_ns = profile_now_ns();

    // Commit the updated pheromone buffers for the next iteration.
    phen.update();
    const std::uint64_t t3_ns = profile_now_ns();

    if (iter_timing != nullptr) {
        iter_timing->k4_ns += (t2_ns - t1_ns);
        iter_timing->k5_ns += (t3_ns - t2_ns);
    }
}

void advance_ants_soa_range(const fractal_land& land, pheronome& phen,
                            std::int32_t nest_x, std::int32_t nest_y,
                            std::int32_t food_x, std::int32_t food_y,
                            AntsSoA& ants, std::size_t begin, std::size_t end,
                            double eps, std::size_t& food_counter, TimingProfile& profile,
                            IterTimingNs* iter_timing)
{
    (void)profile;
    // Clamp the requested range so shared helpers can call this safely.
    if (begin > ants.size()) {
        begin = ants.size();
    }
    if (end > ants.size()) {
        end = ants.size();
    }
    if (end < begin) {
        end = begin;
    }

    const std::uint64_t t0_ns = profile_now_ns();
    std::uint64_t k2_work_ns = 0;
    std::uint64_t k3_work_ns = 0;
    const pheronome& phen_read = phen;
    // Route each SoA mark directly to the shared pheromone map in the serial path.
    auto mark_sink = [&phen](std::int32_t mark_x, std::int32_t mark_y, int /*ind_pher*/) {
        phen.mark_pheronome(mark_x, mark_y);
    };
    // Reuse the same single-ant SoA core for each ant in the selected range.
    for (std::size_t i = begin; i < end; ++i) {
        advance_one_ant_soa_core(land, phen_read, nest_x, nest_y, food_x, food_y, ants, i, eps, food_counter,
                                 mark_sink, k2_work_ns, k3_work_ns);
    }
    const std::uint64_t t1_ns = profile_now_ns();

    // Publish wall-time and work-time counters for the surrounding runner/backend.
    if (iter_timing != nullptr) {
        iter_timing->k1_ns += (t1_ns - t0_ns);
        iter_timing->k2_ns += k2_work_ns;
        iter_timing->k3_ns += k3_work_ns;
    }
}
