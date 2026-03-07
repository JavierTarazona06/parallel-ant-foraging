#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "ant.hpp"
#include "ants_soa.hpp"
#include "app/cli.hpp"
#include "fractal_land.hpp"
#include "pheromone.hpp"
#include "rand_generator.hpp"
#include "renderer.hpp"
#include "sim/backend.hpp"
#include "sim/runner.hpp"
#include "timing_profile.hpp"
#include "window.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

std::size_t configure_thread_count(const RunConfig& run_config)
{
    if (run_config.exec_model != ExecModel::omp) {
        return 1u;
    }

#ifdef _OPENMP
    if (run_config.threads.has_value()) {
        const std::size_t requested = run_config.threads.value();
        const std::size_t capped = std::min(requested, static_cast<std::size_t>(std::numeric_limits<int>::max()));
        omp_set_num_threads(static_cast<int>(capped));
    }
    const int active_threads = omp_get_max_threads();
    return (active_threads > 0) ? static_cast<std::size_t>(active_threads) : 1u;
#else
    return run_config.threads.value_or(1u);
#endif
}

void print_startup_header(const RunConfig& run_config, const SimConfig& sim_config, std::size_t thread_count)
{
    if (run_config.benchmark) {
        std::cout << "METRIC exec " << exec_model_to_text(run_config.exec_model) << '\n';
        std::cout << "METRIC threads " << thread_count << '\n';
        std::cout << "METRIC layout " << layout_to_text(run_config.layout) << '\n';
        std::cout << "METRIC ants " << sim_config.ants << '\n';
        std::cout << "METRIC alpha " << sim_config.alpha << '\n';
        std::cout << "METRIC beta " << sim_config.beta << '\n';
        std::cout << "METRIC epsilon " << sim_config.epsilon << '\n';
        std::cout << "METRIC seed " << sim_config.seed << '\n';
        std::cout << "METRIC init " << init_mode_to_text(sim_config.init_mode) << '\n';
    } else {
        std::cout << "INFO exec=" << exec_model_to_text(run_config.exec_model)
                  << " threads=" << thread_count
                  << " layout=" << layout_to_text(run_config.layout)
                  << " ants=" << sim_config.ants
                  << " alpha=" << sim_config.alpha
                  << " beta=" << sim_config.beta
                  << " epsilon=" << sim_config.epsilon
                  << " seed=" << sim_config.seed
                  << " init=" << init_mode_to_text(sim_config.init_mode) << std::endl;
    }
}

void normalize_land(fractal_land& land)
{
    double max_val = 0.0;
    double min_val = 0.0;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            max_val = std::max(max_val, land(i, j));
            min_val = std::min(min_val, land(i, j));
        }
    }

    const double delta = max_val - min_val;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i) {
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            land(i, j) = (land(i, j) - min_val) / delta;
        }
    }
}

void initialize_ants(const RunConfig& run_config,
                     const SimConfig& sim_config,
                     const fractal_land& land,
                     std::vector<ant>& ants_aos,
                     AntsSoA& ants_soa)
{
    std::size_t seed = sim_config.seed;
    const std::size_t nb_ants = sim_config.ants;
    auto gen_ant_pos = [&land, &seed]() { return rand_int32(0, land.dimensions() - 1, seed); };

    if (run_config.layout == AntLayout::aos) {
        ants_aos.reserve(nb_ants);
        for (std::size_t i = 0; i < nb_ants; ++i) {
            std::int32_t ant_x = sim_config.pos_nest.x;
            std::int32_t ant_y = sim_config.pos_nest.y;
            if (sim_config.init_mode == InitMode::uniform) {
                ant_x = gen_ant_pos();
                ant_y = gen_ant_pos();
            } else {
                (void)rand_int32(0, 0, seed);
            }
            ants_aos.emplace_back(position_t{ant_x, ant_y}, seed);
        }
        return;
    }

    ants_soa.reserve(nb_ants);
    for (std::size_t i = 0; i < nb_ants; ++i) {
        std::int32_t ant_x = sim_config.pos_nest.x;
        std::int32_t ant_y = sim_config.pos_nest.y;
        if (sim_config.init_mode == InitMode::uniform) {
            ant_x = gen_ant_pos();
            ant_y = gen_ant_pos();
        } else {
            (void)rand_int32(0, 0, seed);
        }
        const std::uint32_t ant_seed = static_cast<std::uint32_t>(seed);
        ants_soa.push_back(ant_x, ant_y, ant_seed, 0u);
    }
}

} // namespace

int main(int nargs, char* argv[])
{
    const auto parsed_config = parse_args(nargs, argv);
    if (!parsed_config.has_value()) {
        print_usage(argv[0]);
        return 1;
    }

    const RunConfig run_config = parsed_config->run;
    const SimConfig sim_config = parsed_config->sim;
    const std::size_t thread_count = configure_thread_count(run_config);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    print_startup_header(run_config, sim_config, thread_count);

    MeasurementTotals totals;

    const std::uint64_t p0_start_ns = profile_now_ns();
    fractal_land land(8, 2, 1., 1024);
    totals.p0_ns = profile_now_ns() - p0_start_ns;

    const std::uint64_t p1_start_ns = profile_now_ns();
    normalize_land(land);
    totals.p1_ns = profile_now_ns() - p1_start_ns;

    ant::set_exploration_coef(sim_config.epsilon);

    std::vector<ant> ants_aos;
    AntsSoA ants_soa;
    const std::uint64_t p2_start_ns = profile_now_ns();
    initialize_ants(run_config, sim_config, land, ants_aos, ants_soa);
    totals.p2_ns = profile_now_ns() - p2_start_ns;

    pheronome phen(land.dimensions(), sim_config.pos_food, sim_config.pos_nest, sim_config.alpha, sim_config.beta);
    phen.set_openmp_evaporation_enabled(run_config.exec_model == ExecModel::omp);
    std::size_t food_quantity = 0;
    TimingProfile backend_factory_profile;
    backend_factory_profile.reset();
    WorldState backend_factory_world{land, phen, food_quantity, backend_factory_profile, nullptr};
    std::unique_ptr<Backend> backend = make_backend(run_config, sim_config, backend_factory_world, ants_aos, ants_soa);
    if (!backend) {
        SDL_Quit();
        return 1;
    }

    std::unique_ptr<Window> win;
    std::unique_ptr<Renderer> renderer;
    bool render_enabled = run_config.render;
    if (render_enabled) {
        win = std::make_unique<Window>("Ant Simulation", 2 * land.dimensions() + 10, land.dimensions() + 266);
        if (win->is_ready()) {
            renderer = backend->create_renderer(land, phen, sim_config.pos_nest, sim_config.pos_food);
        } else {
            std::cerr << "Renderer unavailable, disabling render timing.\n";
            render_enabled = false;
        }
    }

    if (run_config.benchmark) {
        run_benchmark(run_config, sim_config, land, phen, *backend, render_enabled, renderer.get(), win.get(),
                      food_quantity, totals);
    } else {
        run_interactive(run_config, sim_config, land, phen, *backend, renderer.get(), win.get(), food_quantity);
    }

    SDL_Quit();
    return 0;
}
