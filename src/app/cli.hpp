#pragma once

#include <cstddef>
#include <optional>
#include "../basic_types.hpp"

enum class AntLayout { aos, soa };
enum class ExecModel { serial, omp, mpi1, mpi2 };
enum class InitMode { uniform, nest };

const char* layout_to_text(AntLayout layout);
const char* exec_model_to_text(ExecModel model);
const char* init_mode_to_text(InitMode mode);

struct RunConfig {
    bool benchmark{false};
    std::size_t iterations{1200};
    std::size_t warmup{200};
    bool render{true};
    AntLayout layout{AntLayout::aos};
    ExecModel exec_model{ExecModel::serial};
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

struct ParsedConfig {
    RunConfig run;
    SimConfig sim;
};

void print_usage(const char* exe_name);
std::optional<ParsedConfig> parse_args(int nargs, char* argv[]);
