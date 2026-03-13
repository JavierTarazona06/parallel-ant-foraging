#include "backend.hpp"

#include <iostream>

#include "aos_backend.hpp"
#include "mpi1_soa_backend.hpp"
#include "mpi2_soa_backend.hpp"
#include "omp_soa_backend.hpp"
#include "soa_backend.hpp"

std::unique_ptr<Backend> make_backend(const RunConfig& run_config,
                                      const SimConfig& sim_config,
                                      const WorldState& world_state,
                                      std::vector<ant>& ants_aos,
                                      AntsSoA& ants_soa)
{
    (void)sim_config;
    (void)world_state;

    // Route MPI1 requests to the replicated-map distributed backend.
    if (run_config.exec_model == ExecModel::mpi1) {
        if (run_config.layout == AntLayout::aos) {
            std::cerr << "Execution model 'mpi1' supports only soa.\n";
            return nullptr;
        }
        return std::make_unique<Mpi1SoaBackend>(ants_soa, run_config.mpi_sync_every);
    }

    // Route MPI2 requests to the domain-decomposed distributed backend.
    if (run_config.exec_model == ExecModel::mpi2) {
        if (run_config.layout == AntLayout::aos) {
            std::cerr << "Execution model 'mpi2' supports only soa.\n";
            return nullptr;
        }
        return std::make_unique<Mpi2SoaBackend>(ants_soa);
    }

    // Route serial requests to either the AoS or SoA backend.
    if (run_config.exec_model == ExecModel::serial) {
        if (run_config.layout == AntLayout::aos) {
            return std::make_unique<AosBackend>(ants_aos);
        }
        return std::make_unique<SoaBackend>(ants_soa);
    }

    // Route OpenMP requests to the parallel SoA backend only.
    if (run_config.exec_model == ExecModel::omp) {
        if (run_config.layout == AntLayout::aos) {
            std::cerr << "Execution model 'omp' is currently supported only with --layout soa.\n";
            return nullptr;
        }
        return std::make_unique<OmpSoaBackend>(ants_soa);
    }

    // Reject any execution model that has no concrete backend implementation.
    std::cerr << "Execution model '" << exec_model_to_text(run_config.exec_model)
              << "' is not implemented yet.\n";
    return nullptr;
}
