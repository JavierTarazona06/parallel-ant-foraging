#include "backend.hpp"

#include <iostream>

#include "aos_backend.hpp"
#include "soa_backend.hpp"

std::unique_ptr<Backend> make_backend(const RunConfig& run_config,
                                      const SimConfig& sim_config,
                                      const WorldState& world_state,
                                      std::vector<ant>& ants_aos,
                                      AntsSoA& ants_soa)
{
    (void)sim_config;
    (void)world_state;

    if (run_config.exec_model == ExecModel::mpi1 || run_config.exec_model == ExecModel::mpi2) {
        std::cerr << "Execution model '" << exec_model_to_text(run_config.exec_model)
                  << "' is not implemented yet. Use --exec serial or --exec omp.\n";
        return nullptr;
    }

    if (run_config.layout == AntLayout::aos) {
        return std::make_unique<AosBackend>(ants_aos);
    }
    return std::make_unique<SoaBackend>(ants_soa);
}
