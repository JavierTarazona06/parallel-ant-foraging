#include "runner.hpp"

#include <iostream>

#include <SDL2/SDL.h>

#include "../fractal_land.hpp"
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
    TimingProfile profile;
    profile.reset();
    SDL_Event event;

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

        if (measured) {
            totals.k0_ns += (k0_end_ns - k0_start_ns);
            totals.k1_ns += iter_timing.k1_ns;
            totals.k4_ns += iter_timing.k4_ns;
            totals.k5_ns += iter_timing.k5_ns;
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
    totals.k2_ns = profile.totals().k2_ns;
    totals.k3_ns = profile.totals().k3_ns;

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
    std::cout << "METRIC r0_ns " << totals.r0_ns << '\n';
    std::cout << "METRIC e0_ns " << totals.e0_ns << '\n';
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
