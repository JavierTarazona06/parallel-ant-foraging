#pragma once

#include <cstddef>
#include <optional>
#include "../basic_types.hpp"

enum class AntLayout { aos, soa };
enum class ExecModel { serial, omp, mpi1, mpi2 };
enum class InitMode { uniform, nest };

// Convert enum values back to CLI-friendly text for logs and METRIC headers.
const char* layout_to_text(AntLayout layout);
const char* exec_model_to_text(ExecModel model);
const char* init_mode_to_text(InitMode mode);

struct RunConfig {
    // Hold execution-mode and benchmark-loop settings parsed from the CLI.
    bool benchmark{false};
    std::size_t iterations{1200};
    std::size_t warmup{200};
    bool render{true};
    AntLayout layout{AntLayout::aos};
    ExecModel exec_model{ExecModel::serial};
    std::size_t mpi_sync_every{1};
    std::optional<std::size_t> threads{};
};

struct SimConfig {
    // Hold model parameters that affect ants, pheromones, and initialization.
    std::size_t ants{5000};
    std::size_t seed{2026};
    double alpha{0.7};
    double beta{0.999};
    double epsilon{0.8};
    InitMode init_mode{InitMode::uniform};
    position_t pos_nest{256, 256};
    position_t pos_food{500, 500};
};

struct ParsedConfig {
    // Return both config layers together after CLI parsing succeeds.
    RunConfig run;
    SimConfig sim;
};

// Print the supported command-line syntax for the executable.
void print_usage(const char* exe_name);
// Parse CLI arguments into validated run and simulation configs.
std::optional<ParsedConfig> parse_args(int nargs, char* argv[]);
