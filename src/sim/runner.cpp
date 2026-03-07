#include "runner.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>

#include <SDL2/SDL.h>

#include "../fractal_land.hpp"
#include "../mpi/mpi_runtime.hpp"
#include "../timing_profile.hpp"
#include "backend.hpp"
#include "../renderer.hpp"
#include "../window.hpp"

void run_benchmark(const RunConfig& run_config,
                   const SimConfig& sim_config,
                   const fractal_land& land,
                   pheronome& phen,
                   Backend& backend,
                   bool render_enabled,
                   Renderer* renderer,
                   Window* win,
                   std::size_t& food_quantity,
                   MeasurementTotals& totals)
{
    const bool mpi_mode = (run_config.exec_model == ExecModel::mpi1);
    TimingProfile profile;
    profile.reset();
    SDL_Event event;
    std::size_t previous_food_quantity = food_quantity;

    for (std::size_t it = 0; it < run_config.iterations; ++it) {
        const bool measured = (it >= run_config.warmup);
        profile.set_enabled(measured);

        const std::uint64_t e0_start_ns = profile_now_ns();
        while (SDL_PollEvent(&event)) {
        }
        const std::uint64_t e0_end_ns = profile_now_ns();
        if (measured) {
            totals.e0_ns += (e0_end_ns - e0_start_ns);
        }

        IterTimingNs iter_timing{};
        const std::uint64_t k0_start_ns = profile_now_ns();
        WorldState world{land, phen, food_quantity, profile, &iter_timing};
        backend.step(world, sim_config);
        const std::uint64_t k0_end_ns = profile_now_ns();

        if (food_quantity < previous_food_quantity) {
            totals.food_monotonic_ok = false;
        }
        previous_food_quantity = food_quantity;

        if (measured) {
            totals.k0_ns += (k0_end_ns - k0_start_ns);
            totals.k1_ns += iter_timing.k1_ns;
            totals.k2_ns += iter_timing.k2_ns;
            totals.k3_ns += iter_timing.k3_ns;
            totals.k4_ns += iter_timing.k4_ns;
            totals.k5_ns += iter_timing.k5_ns;
            totals.k_mpi_sync_ns += iter_timing.k_mpi_sync_ns;
            totals.touched_raw_total += iter_timing.touched_raw_count;
            totals.touched_unique_total += iter_timing.touched_unique_count;
            totals.measured_iterations += 1;
        }

        if (render_enabled && renderer != nullptr && win != nullptr) {
            const std::uint64_t r0_start_ns = profile_now_ns();
            renderer->display(*win, food_quantity);
            win->blit();
            const std::uint64_t r0_end_ns = profile_now_ns();
            if (measured) {
                totals.r0_ns += (r0_end_ns - r0_start_ns);
            }
        }
    }

    profile.set_enabled(false);
    totals.k2_ns += profile.totals().k2_ns;
    totals.k3_ns += profile.totals().k3_ns;
    totals.food_quantity_final = food_quantity;

    const std::vector<double>& v1 = phen.v1_buffer();
    const std::vector<double>& v2 = phen.v2_buffer();
    totals.phen_v1_sum = std::accumulate(v1.begin(), v1.end(), 0.0);
    totals.phen_v2_sum = std::accumulate(v2.begin(), v2.end(), 0.0);
    totals.phen_v1_max = (v1.empty() ? 0.0 : *std::max_element(v1.begin(), v1.end()));
    totals.phen_v2_max = (v2.empty() ? 0.0 : *std::max_element(v2.begin(), v2.end()));

    if (mpi_mode) {
        totals.p0_ns = mpi_runtime::allreduce_max_uint64(totals.p0_ns);
        totals.p1_ns = mpi_runtime::allreduce_max_uint64(totals.p1_ns);
        totals.p2_ns = mpi_runtime::allreduce_max_uint64(totals.p2_ns);
        totals.k0_ns = mpi_runtime::allreduce_max_uint64(totals.k0_ns);
        totals.k1_ns = mpi_runtime::allreduce_max_uint64(totals.k1_ns);
        totals.k2_ns = mpi_runtime::allreduce_max_uint64(totals.k2_ns);
        totals.k3_ns = mpi_runtime::allreduce_max_uint64(totals.k3_ns);
        totals.k4_ns = mpi_runtime::allreduce_max_uint64(totals.k4_ns);
        totals.k5_ns = mpi_runtime::allreduce_max_uint64(totals.k5_ns);
        totals.k_mpi_sync_ns = mpi_runtime::allreduce_max_uint64(totals.k_mpi_sync_ns);
        totals.r0_ns = mpi_runtime::allreduce_max_uint64(totals.r0_ns);
        totals.e0_ns = mpi_runtime::allreduce_max_uint64(totals.e0_ns);
        totals.touched_raw_total = mpi_runtime::allreduce_sum_uint64(totals.touched_raw_total);
        totals.touched_unique_total = mpi_runtime::allreduce_sum_uint64(totals.touched_unique_total);
        totals.food_quantity_final = static_cast<std::size_t>(
            mpi_runtime::allreduce_max_uint64(static_cast<std::uint64_t>(totals.food_quantity_final)));
        const std::uint64_t monotonic_ok_local = totals.food_monotonic_ok ? 1u : 0u;
        const std::uint64_t monotonic_ok_count = mpi_runtime::allreduce_sum_uint64(monotonic_ok_local);
        totals.food_monotonic_ok = (monotonic_ok_count == static_cast<std::uint64_t>(mpi_runtime::size()));
    }

    if (mpi_mode && !mpi_runtime::is_root()) {
        return;
    }

    std::cout << "METRIC measured_iterations " << totals.measured_iterations << '\n';
    std::cout << "METRIC total_iterations " << run_config.iterations << '\n';
    std::cout << "METRIC warmup_iterations " << run_config.warmup << '\n';
    std::cout << "METRIC nb_ants " << sim_config.ants << '\n';
    std::cout << "METRIC render_enabled " << (render_enabled ? 1 : 0) << '\n';
    std::cout << "METRIC p0_ns " << totals.p0_ns << '\n';
    std::cout << "METRIC p1_ns " << totals.p1_ns << '\n';
    std::cout << "METRIC p2_ns " << totals.p2_ns << '\n';
    std::cout << "METRIC k0_ns " << totals.k0_ns << '\n';
    std::cout << "METRIC k1_ns " << totals.k1_ns << '\n';
    std::cout << "METRIC k2_ns " << totals.k2_ns << '\n';
    std::cout << "METRIC k3_ns " << totals.k3_ns << '\n';
    std::cout << "METRIC k4_ns " << totals.k4_ns << '\n';
    std::cout << "METRIC k5_ns " << totals.k5_ns << '\n';
    if (mpi_mode) {
        std::cout << "METRIC k_mpi_sync_ns " << totals.k_mpi_sync_ns << '\n';
    }
    std::cout << "METRIC r0_ns " << totals.r0_ns << '\n';
    std::cout << "METRIC e0_ns " << totals.e0_ns << '\n';
    std::cout << "METRIC touched_raw_total " << totals.touched_raw_total << '\n';
    std::cout << "METRIC touched_unique_total " << totals.touched_unique_total << '\n';
    const double measured_iter = static_cast<double>(totals.measured_iterations);
    const double touched_raw_per_iter =
        (totals.measured_iterations > 0) ? (static_cast<double>(totals.touched_raw_total) / measured_iter) : 0.0;
    const double touched_unique_per_iter =
        (totals.measured_iterations > 0) ? (static_cast<double>(totals.touched_unique_total) / measured_iter) : 0.0;
    const double touched_unique_ratio =
        (totals.touched_raw_total > 0)
            ? (static_cast<double>(totals.touched_unique_total) / static_cast<double>(totals.touched_raw_total))
            : 0.0;
    std::cout << "METRIC touched_raw_per_iter " << touched_raw_per_iter << '\n';
    std::cout << "METRIC touched_unique_per_iter " << touched_unique_per_iter << '\n';
    std::cout << "METRIC touched_unique_ratio " << touched_unique_ratio << '\n';
    std::cout << "METRIC food_monotonic_ok " << (totals.food_monotonic_ok ? 1 : 0) << '\n';
    std::cout << "METRIC food_quantity_final " << totals.food_quantity_final << '\n';
    std::cout << "METRIC phen_v1_sum " << totals.phen_v1_sum << '\n';
    std::cout << "METRIC phen_v2_sum " << totals.phen_v2_sum << '\n';
    std::cout << "METRIC phen_v1_max " << totals.phen_v1_max << '\n';
    std::cout << "METRIC phen_v2_max " << totals.phen_v2_max << '\n';
}

void run_interactive(const RunConfig& run_config,
                     const SimConfig& sim_config,
                     const fractal_land& land,
                     pheronome& phen,
                     Backend& backend,
                     Renderer* renderer,
                     Window* win,
                     std::size_t& food_quantity)
{
    (void)run_config;
    TimingProfile profile;
    profile.reset();
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;

    while (cont_loop) {
        ++it;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                cont_loop = false;
            }
        }
        WorldState world{land, phen, food_quantity, profile, nullptr};
        backend.step(world, sim_config);
        if (renderer != nullptr && win != nullptr) {
            renderer->display(*win, food_quantity);
            win->blit();
        }
        if (not_food_in_nest && food_quantity > 0) {
            std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
    }
}
