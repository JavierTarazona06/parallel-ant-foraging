#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"
#include "timing_profile.hpp"

struct IterTimingNs {
    std::uint64_t k1_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
};

struct RunConfig {
    bool benchmark{false};
    std::size_t iterations{1200};
    std::size_t warmup{200};
    bool render{true};
};

struct MeasurementTotals {
    std::uint64_t p0_ns{0};
    std::uint64_t p1_ns{0};
    std::uint64_t p2_ns{0};
    std::uint64_t k0_ns{0};
    std::uint64_t k1_ns{0};
    std::uint64_t k2_ns{0};
    std::uint64_t k3_ns{0};
    std::uint64_t k4_ns{0};
    std::uint64_t k5_ns{0};
    std::uint64_t r0_ns{0};
    std::uint64_t e0_ns{0};
    std::size_t measured_iterations{0};
};

void print_usage(const char* exe_name)
{
    std::cout << "Usage: " << exe_name << " [--benchmark] [--iterations N] [--warmup N] [--no-render]\n";
}

bool parse_size_value(const char* text, std::size_t& value_out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }
    errno = 0;
    char* end_ptr = nullptr;
    unsigned long long parsed = std::strtoull(text, &end_ptr, 10);
    if (errno != 0 || end_ptr == text || *end_ptr != '\0') {
        return false;
    }
    value_out = static_cast<std::size_t>(parsed);
    return true;
}

bool parse_args(int nargs, char* argv[], RunConfig& config)
{
    for (int i = 1; i < nargs; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--benchmark") {
            config.benchmark = true;
            continue;
        }
        if (arg == "--no-render") {
            config.render = false;
            continue;
        }
        if (arg == "--iterations") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], config.iterations)) {
                std::cerr << "Invalid value for --iterations\n";
                return false;
            }
            continue;
        }
        if (arg == "--warmup") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], config.warmup)) {
                std::cerr << "Invalid value for --warmup\n";
                return false;
            }
            continue;
        }
        std::cerr << "Unknown argument: " << arg << '\n';
        return false;
    }
    if (config.benchmark && config.warmup >= config.iterations) {
        std::cerr << "Warmup must be strictly lower than iterations\n";
        return false;
    }
    return true;
}

void advance_time(const fractal_land& land, pheronome& phen,
                  const position_t& pos_nest, const position_t& pos_food,
                  std::vector<ant>& ants, std::size_t& cpteur,
                  IterTimingNs* iter_timing = nullptr)
{
    std::uint64_t t0_ns = profile_now_ns();
    for (size_t i = 0; i < ants.size(); ++i)
        ants[i].advance(phen, land, pos_food, pos_nest, cpteur);
    std::uint64_t t1_ns = profile_now_ns();

    phen.do_evaporation();
    std::uint64_t t2_ns = profile_now_ns();

    phen.update();
    std::uint64_t t3_ns = profile_now_ns();

    if (iter_timing != nullptr) {
        iter_timing->k1_ns += (t1_ns - t0_ns);
        iter_timing->k4_ns += (t2_ns - t1_ns);
        iter_timing->k5_ns += (t3_ns - t2_ns);
    }
}

int main(int nargs, char* argv[])
{
    RunConfig config;
    if (!parse_args(nargs, argv, config)) {
        print_usage(argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    MeasurementTotals totals;
    g_timing_profile_totals = TimingProfileTotals{};
    g_timing_profile_enabled = false;

    std::size_t seed = 2026;
    const int nb_ants = 5000;
    const double eps = 0.8;
    const double alpha = 0.7;
    const double beta = 0.999;
    position_t pos_nest{256, 256};
    position_t pos_food{500, 500};

    std::uint64_t p0_start_ns = profile_now_ns();
    fractal_land land(8, 2, 1., 1024);
    totals.p0_ns = profile_now_ns() - p0_start_ns;

    std::uint64_t p1_start_ns = profile_now_ns();
    double max_val = 0.0;
    double min_val = 0.0;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i)
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j) {
            max_val = std::max(max_val, land(i, j));
            min_val = std::min(min_val, land(i, j));
        }
    double delta = max_val - min_val;
    for (fractal_land::dim_t i = 0; i < land.dimensions(); ++i)
        for (fractal_land::dim_t j = 0; j < land.dimensions(); ++j)
            land(i, j) = (land(i, j) - min_val) / delta;
    totals.p1_ns = profile_now_ns() - p1_start_ns;

    ant::set_exploration_coef(eps);
    std::uint64_t p2_start_ns = profile_now_ns();
    std::vector<ant> ants;
    ants.reserve(nb_ants);
    auto gen_ant_pos = [&land, &seed]() { return rand_int32(0, land.dimensions() - 1, seed); };
    for (size_t i = 0; i < static_cast<size_t>(nb_ants); ++i)
        ants.emplace_back(position_t{gen_ant_pos(), gen_ant_pos()}, seed);
    totals.p2_ns = profile_now_ns() - p2_start_ns;

    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);

    std::unique_ptr<Window> win;
    std::unique_ptr<Renderer> renderer;
    bool render_enabled = config.render;
    if (render_enabled) {
        win = std::make_unique<Window>("Ant Simulation", 2 * land.dimensions() + 10, land.dimensions() + 266);
        if (win->is_ready()) {
            renderer = std::make_unique<Renderer>(land, phen, pos_nest, pos_food, ants);
        } else {
            std::cerr << "Renderer unavailable, disabling render timing.\n";
            render_enabled = false;
        }
    }

    size_t food_quantity = 0;
    SDL_Event event;

    if (config.benchmark) {
        for (std::size_t it = 0; it < config.iterations; ++it) {
            const bool measured = (it >= config.warmup);
            g_timing_profile_enabled = measured;

            std::uint64_t e0_start_ns = profile_now_ns();
            while (SDL_PollEvent(&event)) {
            }
            std::uint64_t e0_end_ns = profile_now_ns();
            if (measured) {
                totals.e0_ns += (e0_end_ns - e0_start_ns);
            }

            IterTimingNs iter_timing{};
            std::uint64_t k0_start_ns = profile_now_ns();
            advance_time(land, phen, pos_nest, pos_food, ants, food_quantity, &iter_timing);
            std::uint64_t k0_end_ns = profile_now_ns();

            if (measured) {
                totals.k0_ns += (k0_end_ns - k0_start_ns);
                totals.k1_ns += iter_timing.k1_ns;
                totals.k4_ns += iter_timing.k4_ns;
                totals.k5_ns += iter_timing.k5_ns;
                totals.measured_iterations += 1;
            }

            if (render_enabled && renderer && win) {
                std::uint64_t r0_start_ns = profile_now_ns();
                renderer->display(*win, food_quantity);
                win->blit();
                std::uint64_t r0_end_ns = profile_now_ns();
                if (measured) {
                    totals.r0_ns += (r0_end_ns - r0_start_ns);
                }
            }
        }

        g_timing_profile_enabled = false;
        totals.k2_ns = g_timing_profile_totals.k2_ns;
        totals.k3_ns = g_timing_profile_totals.k3_ns;

        std::cout << "METRIC measured_iterations " << totals.measured_iterations << '\n';
        std::cout << "METRIC total_iterations " << config.iterations << '\n';
        std::cout << "METRIC warmup_iterations " << config.warmup << '\n';
        std::cout << "METRIC nb_ants " << nb_ants << '\n';
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
    } else {
        bool cont_loop = true;
        bool not_food_in_nest = true;
        std::size_t it = 0;
        while (cont_loop) {
            ++it;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT)
                    cont_loop = false;
            }
            advance_time(land, phen, pos_nest, pos_food, ants, food_quantity);
            if (renderer && win) {
                renderer->display(*win, food_quantity);
                win->blit();
            }
            if (not_food_in_nest && food_quantity > 0) {
                std::cout << "La première nourriture est arrivée au nid a l'iteration " << it << std::endl;
                not_food_in_nest = false;
            }
        }
    }

    SDL_Quit();
    return 0;
}
