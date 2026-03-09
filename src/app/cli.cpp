#include "cli.hpp"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>

const char* layout_to_text(AntLayout layout)
{
    return (layout == AntLayout::soa) ? "soa" : "aos";
}

const char* exec_model_to_text(ExecModel model)
{
    switch (model) {
    case ExecModel::serial:
        return "serial";
    case ExecModel::omp:
        return "omp";
    case ExecModel::mpi1:
        return "mpi1";
    case ExecModel::mpi2:
        return "mpi2";
    }
    return "serial";
}

const char* init_mode_to_text(InitMode mode)
{
    return (mode == InitMode::nest) ? "nest" : "uniform";
}

void print_usage(const char* exe_name)
{
    std::cout << "Usage: " << exe_name
              << " [--benchmark] [--iterations N] [--warmup N] [--no-render] [--layout <aos|soa>]"
              << " [--exec <serial|omp|mpi1|mpi2>]"
              << " [--mpi-sync-every K]"
              << " [--threads N]"
              << " [--ants N] [--seed N] [--alpha X] [--beta X] [--epsilon X] [--init <nest|uniform>]\n";
}

namespace {

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

bool parse_positive_size_value(const char* text, std::size_t& value_out)
{
    if (!parse_size_value(text, value_out)) {
        return false;
    }
    return value_out > 0;
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

bool parse_exec_model_value(const std::string& text, ExecModel& model_out)
{
    if (text == "serial") {
        model_out = ExecModel::serial;
        return true;
    }
    if (text == "omp") {
        model_out = ExecModel::omp;
        return true;
    }
    if (text == "mpi1") {
        model_out = ExecModel::mpi1;
        return true;
    }
    if (text == "mpi2") {
        model_out = ExecModel::mpi2;
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

} // namespace

std::optional<ParsedConfig> parse_args(int nargs, char* argv[])
{
    ParsedConfig parsed{};
    for (int i = 1; i < nargs; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--benchmark") {
            parsed.run.benchmark = true;
            continue;
        }
        if (arg == "--no-render") {
            parsed.run.render = false;
            continue;
        }
        if (arg == "--iterations") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], parsed.run.iterations)) {
                std::cerr << "Invalid value for --iterations\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--warmup") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], parsed.run.warmup)) {
                std::cerr << "Invalid value for --warmup\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--layout") {
            if (i + 1 >= nargs || !parse_layout_value(argv[++i], parsed.run.layout)) {
                std::cerr << "Invalid value for --layout (expected aos|soa)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--layout=", 0) == 0) {
            if (!parse_layout_value(arg.substr(9), parsed.run.layout)) {
                std::cerr << "Invalid value for --layout (expected aos|soa)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--exec") {
            if (i + 1 >= nargs || !parse_exec_model_value(argv[++i], parsed.run.exec_model)) {
                std::cerr << "Invalid value for --exec (expected serial|omp|mpi1|mpi2)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--exec=", 0) == 0) {
            if (!parse_exec_model_value(arg.substr(7), parsed.run.exec_model)) {
                std::cerr << "Invalid value for --exec (expected serial|omp|mpi1|mpi2)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--mpi-sync-every") {
            if (i + 1 >= nargs || !parse_positive_size_value(argv[++i], parsed.run.mpi_sync_every)) {
                std::cerr << "Invalid value for --mpi-sync-every (expected integer >= 1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--mpi-sync-every=", 0) == 0) {
            if (!parse_positive_size_value(arg.substr(17).c_str(), parsed.run.mpi_sync_every)) {
                std::cerr << "Invalid value for --mpi-sync-every (expected integer >= 1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--threads") {
            std::size_t threads = 0;
            if (i + 1 >= nargs || !parse_positive_size_value(argv[++i], threads)) {
                std::cerr << "Invalid value for --threads (expected integer > 0)\n";
                return std::nullopt;
            }
            parsed.run.threads = threads;
            continue;
        }
        if (arg.rfind("--threads=", 0) == 0) {
            std::size_t threads = 0;
            if (!parse_positive_size_value(arg.substr(10).c_str(), threads)) {
                std::cerr << "Invalid value for --threads (expected integer > 0)\n";
                return std::nullopt;
            }
            parsed.run.threads = threads;
            continue;
        }
        if (arg == "--ants") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], parsed.sim.ants)) {
                std::cerr << "Invalid value for --ants\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--ants=", 0) == 0) {
            if (!parse_size_value(arg.substr(7).c_str(), parsed.sim.ants)) {
                std::cerr << "Invalid value for --ants\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--seed") {
            if (i + 1 >= nargs || !parse_size_value(argv[++i], parsed.sim.seed)) {
                std::cerr << "Invalid value for --seed\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--seed=", 0) == 0) {
            if (!parse_size_value(arg.substr(7).c_str(), parsed.sim.seed)) {
                std::cerr << "Invalid value for --seed\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--alpha") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], parsed.sim.alpha)) {
                std::cerr << "Invalid value for --alpha (expected 0<=alpha<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--alpha=", 0) == 0) {
            if (!parse_probability_value(arg.substr(8).c_str(), parsed.sim.alpha)) {
                std::cerr << "Invalid value for --alpha (expected 0<=alpha<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--beta") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], parsed.sim.beta)) {
                std::cerr << "Invalid value for --beta (expected 0<=beta<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--beta=", 0) == 0) {
            if (!parse_probability_value(arg.substr(7).c_str(), parsed.sim.beta)) {
                std::cerr << "Invalid value for --beta (expected 0<=beta<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--epsilon") {
            if (i + 1 >= nargs || !parse_probability_value(argv[++i], parsed.sim.epsilon)) {
                std::cerr << "Invalid value for --epsilon (expected 0<=epsilon<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--epsilon=", 0) == 0) {
            if (!parse_probability_value(arg.substr(10).c_str(), parsed.sim.epsilon)) {
                std::cerr << "Invalid value for --epsilon (expected 0<=epsilon<=1)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg == "--init") {
            if (i + 1 >= nargs || !parse_init_mode_value(argv[++i], parsed.sim.init_mode)) {
                std::cerr << "Invalid value for --init (expected nest|uniform)\n";
                return std::nullopt;
            }
            continue;
        }
        if (arg.rfind("--init=", 0) == 0) {
            if (!parse_init_mode_value(arg.substr(7), parsed.sim.init_mode)) {
                std::cerr << "Invalid value for --init (expected nest|uniform)\n";
                return std::nullopt;
            }
            continue;
        }
        std::cerr << "Unknown argument: " << arg << '\n';
        return std::nullopt;
    }

    if (parsed.run.benchmark && parsed.run.warmup >= parsed.run.iterations) {
        std::cerr << "Warmup must be strictly lower than iterations\n";
        return std::nullopt;
    }
    return parsed;
}
