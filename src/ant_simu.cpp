#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "fractal_land.hpp"
#include "ant.hpp"
#include "ants_soa.hpp"
#include "pheromone.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"
#include "timing_profile.hpp"

enum class AntLayout { aos, soa };
enum class InitMode { uniform, nest };

const char* layout_to_text(AntLayout layout)
{
    return (layout == AntLayout::soa) ? "soa" : "aos";
}

const char* init_mode_to_text(InitMode mode)
{
    return (mode == InitMode::nest) ? "nest" : "uniform";
}

struct RunConfig {
    bool benchmark{false};
    std::size_t iterations{1200};
    std::size_t warmup{200};
    bool render{true};
    AntLayout layout{AntLayout::aos};
};

struct SimConfig {
    std::size_t ants{5000};
    std::size_t seed{2026};
    double alpha{0.7};
    double beta{0.999};
    double epsilon{0.8};
    InitMode init_mode{InitMode::uniform};
    position_t pos_nest{256, 256};
    position_t pos_food{500, 500};
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
    std::cout << "Usage: " << exe_name
              << " [--benchmark] [--iterations N] [--warmup N] [--no-render] [--layout <aos|soa>]"
              << " [--ants N] [--seed N] [--alpha X] [--beta X] [--epsilon X] [--init <nest|uniform>]\n";
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

bool parse_layout_value(const std::string& text, AntLayout& layout_out)
{
    if (text == "aos") {
        layout_out = AntLayout::aos;
        return true;
    }
    if (text == "soa") {
        layout_out = AntLayout::soa;
        return true;
    }
    return false;
}

bool parse_init_mode_value(const std::string& text, InitMode& mode_out)
{
    if (text == "uniform") {
        mode_out = InitMode::uniform;
        return true;
    }
    if (text == "nest") {
        mode_out = InitMode::nest;
        return true;
    }
    return false;
}

bool parse_double_value(const char* text, double& value_out)
{
    if (text == nullptr || *text == '\0') {
        return false;
    }
    errno = 0;
    char* end_ptr = nullptr;
    const double parsed = std::strtod(text, &end_ptr);
    if (errno != 0 || end_ptr == text || *end_ptr != '\0') {
        return false;
    }
    value_out = parsed;
    return true;
}

bool parse_probability_value(const char* text, double& value_out)
{
    if (!parse_double_value(text, value_out)) {
        return false;
    }
    return (value_out >= 0.0) && (value_out <= 1.0);
}

bool parse_args(int nargs, char* argv[], RunConfig& run_config, SimConfig& sim_config)
{
    for (int i = 1; i < nargs; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--benchmark") {
            run_config.benchmark = true;
            continue;
        }
        if (arg == "--no-render") {
            run_config.render = false;
            continue;
        }
        if (arg == "--iterations") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], run_config.iterations)) {
                std::cerr << "Invalid value for --iterations\n";
                return false;
            }
            continue;
        }
        if (arg == "--warmup") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], run_config.warmup)) {
                std::cerr << "Invalid value for --warmup\n";
                return false;
            }
            continue;
        }
        if (arg == "--layout") {
            if (i + 1 >= nargs || !parse_layout_value(argv[++i], run_config.layout)) {
                std::cerr << "Invalid value for --layout (expected aos|soa)\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--layout=", 0) == 0) {
            if (!parse_layout_value(arg.substr(9), run_config.layout)) {
                std::cerr << "Invalid value for --layout (expected aos|soa)\n";
                return false;
            }
            continue;
        }
        if (arg == "--ants") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], sim_config.ants)) {
                std::cerr << "Invalid value for --ants\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--ants=", 0) == 0) {
            if (!parse_size_value(arg.substr(7).c_str(), sim_config.ants)) {
                std::cerr << "Invalid value for --ants\n";
                return false;
            }
            continue;
        }
        if (arg == "--seed") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], sim_config.seed)) {
                std::cerr << "Invalid value for --seed\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--seed=", 0) == 0) {
            if (!parse_size_value(arg.substr(7).c_str(), sim_config.seed)) {
                std::cerr << "Invalid value for --seed\n";
                return false;
            }
            continue;
        }
        if (arg == "--alpha") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], sim_config.alpha)) {
                std::cerr << "Invalid value for --alpha (expected 0<=alpha<=1)\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--alpha=", 0) == 0) {
            if (!parse_probability_value(arg.substr(8).c_str(), sim_config.alpha)) {
                std::cerr << "Invalid value for --alpha (expected 0<=alpha<=1)\n";
                return false;
            }
            continue;
        }
        if (arg == "--beta") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], sim_config.beta)) {
                std::cerr << "Invalid value for --beta (expected 0<=beta<=1)\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--beta=", 0) == 0) {
            if (!parse_probability_value(arg.substr(7).c_str(), sim_config.beta)) {
                std::cerr << "Invalid value for --beta (expected 0<=beta<=1)\n";
                return false;
            }
            continue;
        }
        if (arg == "--epsilon") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], sim_config.epsilon)) {
                std::cerr << "Invalid value for --epsilon (expected 0<=epsilon<=1)\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--epsilon=", 0) == 0) {
            if (!parse_probability_value(arg.substr(10).c_str(), sim_config.epsilon)) {
                std::cerr << "Invalid value for --epsilon (expected 0<=epsilon<=1)\n";
                return false;
            }
            continue;
        }
        if (arg == "--init") {
            if (i + 1 >= nargs || !parse_init_mode_value(argv[++i], sim_config.init_mode)) {
                std::cerr << "Invalid value for --init (expected nest|uniform)\n";
                return false;
            }
            continue;
        }
        if (arg.rfind("--init=", 0) == 0) {
            if (!parse_init_mode_value(arg.substr(7), sim_config.init_mode)) {
                std::cerr << "Invalid value for --init (expected nest|uniform)\n";
                return false;
            }
            continue;
        }
        std::cerr << "Unknown argument: " << arg << '\n';
        return false;
    }
    if (run_config.benchmark && run_config.warmup >= run_config.iterations) {
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
    RunConfig run_config;
    SimConfig sim_config;
    if (!parse_args(nargs, argv, run_config, sim_config)) {
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

    std::size_t seed = sim_config.seed;
    const std::size_t nb_ants = sim_config.ants;
    const double eps = sim_config.epsilon;
    const double alpha = sim_config.alpha;
    const double beta = sim_config.beta;

    if (run_config.benchmark) {
        std::cout << "METRIC layout " << layout_to_text(run_config.layout) << '\n';
        std::cout << "METRIC ants " << nb_ants << '\n';
        std::cout << "METRIC alpha " << alpha << '\n';
        std::cout << "METRIC beta " << beta << '\n';
        std::cout << "METRIC epsilon " << eps << '\n';
        std::cout << "METRIC seed " << seed << '\n';
        std::cout << "METRIC init " << init_mode_to_text(sim_config.init_mode) << '\n';
    } else {
        std::cout << "INFO layout=" << layout_to_text(run_config.layout)
                  << " ants=" << nb_ants
                  << " alpha=" << alpha
                  << " beta=" << beta
                  << " epsilon=" << eps
                  << " seed=" << seed
                  << " init=" << init_mode_to_text(sim_config.init_mode) << std::endl;
    }

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
    std::vector<ant> ants_aos;
    AntsSoA ants_soa;
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
    } else {
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
    totals.p2_ns = profile_now_ns() - p2_start_ns;

    pheronome phen(land.dimensions(), sim_config.pos_food, sim_config.pos_nest, alpha, beta);

    std::unique_ptr<Window> win;
    std::unique_ptr<Renderer> renderer;
    bool render_enabled = run_config.render;
    if (render_enabled) {
        win = std::make_unique<Window>("Ant Simulation", 2 * land.dimensions() + 10, land.dimensions() + 266);
        if (win->is_ready()) {
            if (run_config.layout == AntLayout::aos) {
                renderer = std::make_unique<Renderer>(land, phen, sim_config.pos_nest, sim_config.pos_food, ants_aos);
            } else {
                renderer = std::make_unique<Renderer>(land, phen, sim_config.pos_nest, sim_config.pos_food, ants_soa);
            }
        } else {
            std::cerr << "Renderer unavailable, disabling render timing.\n";
            render_enabled = false;
        }
    }

    size_t food_quantity = 0;
    SDL_Event event;

    if (run_config.benchmark) {
        for (std::size_t it = 0; it < run_config.iterations; ++it) {
            const bool measured = (it >= run_config.warmup);
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
            if (run_config.layout == AntLayout::aos) {
                advance_time(land, phen, sim_config.pos_nest, sim_config.pos_food, ants_aos, food_quantity,
                             &iter_timing);
            } else {
                advance_time_soa(land, phen, sim_config.pos_nest.x, sim_config.pos_nest.y,
                                 sim_config.pos_food.x, sim_config.pos_food.y,
                                 ants_soa, eps, food_quantity, &iter_timing);
            }
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
        std::cout << "METRIC total_iterations " << run_config.iterations << '\n';
        std::cout << "METRIC warmup_iterations " << run_config.warmup << '\n';
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
            if (run_config.layout == AntLayout::aos) {
                advance_time(land, phen, sim_config.pos_nest, sim_config.pos_food, ants_aos, food_quantity);
            } else {
                advance_time_soa(land, phen, sim_config.pos_nest.x, sim_config.pos_nest.y,
                                 sim_config.pos_food.x, sim_config.pos_food.y,
                                 ants_soa, eps, food_quantity);
            }
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
