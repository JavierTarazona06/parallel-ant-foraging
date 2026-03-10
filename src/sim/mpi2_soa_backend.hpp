#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "backend.hpp"
#include "../mpi2/local_pheromone_grid.hpp"

class Mpi2SoaBackend final : public Backend
{
public:
    explicit Mpi2SoaBackend(AntsSoA& ants);

    const char* name() const override;
    void step(WorldState& world, const SimConfig& sim_config) override;
    std::unique_ptr<Renderer> create_renderer(const fractal_land& land,
                                              const pheronome& phen,
                                              const position_t& pos_nest,
                                              const position_t& pos_food) const override;

private:
    struct LocalAntsSoA {
        std::vector<std::int32_t> x;
        std::vector<std::int32_t> y;
        std::vector<std::uint8_t> state;
        std::vector<std::uint32_t> seed;

        std::size_t size() const { return x.size(); }
        bool empty() const { return x.empty(); }
        void clear()
        {
            x.clear();
            y.clear();
            state.clear();
            seed.clear();
        }
        void reserve(std::size_t n)
        {
            x.reserve(n);
            y.reserve(n);
            state.reserve(n);
            seed.reserve(n);
        }
        void push_back(std::int32_t ax, std::int32_t ay, std::uint8_t as, std::uint32_t sd)
        {
            x.push_back(ax);
            y.push_back(ay);
            state.push_back(as);
            seed.push_back(sd);
        }
        void resize(std::size_t n)
        {
            x.resize(n);
            y.resize(n);
            state.resize(n);
            seed.resize(n);
        }
    };

    struct LocalCellCoord {
        std::int32_t lx{0};
        std::int32_t ly{0};
    };

    struct MigrationAntPOD {
        int x{0};
        int y{0};
        int state{0};
        std::uint64_t seed{0};
    };

    void initialize_partition_if_needed(const WorldState& world);
    void initialize_local_ants_if_needed();
    void initialize_local_pheromone_grid(const WorldState& world);
    void clear_halo(LocalPheromoneGrid& grid);
    void prepare_iteration_buffer();
    void commit_iteration_buffer();
    void local_k4_evaporation(LocalPheromoneGrid& grid, double beta);
    void apply_local_forcing(LocalPheromoneGrid& grid, const SimConfig& sim_config);
    void halo_exchange();
    void halo_exchange_channel(double* channel_base, int tag_base);
    std::vector<MigrationAntPOD> pack_outbox(const LocalAntsSoA& outbox) const;
    std::vector<MigrationAntPOD> exchange_direction(const std::vector<MigrationAntPOD>& send_buffer,
                                                    int send_neighbor, int recv_neighbor,
                                                    int tag_base);
    void append_received_ants(const std::vector<MigrationAntPOD>& received);
    void exchange_migrating_ants();
    void sync_global_state_for_render(WorldState& world);
    void step_local_ants(WorldState& world, const SimConfig& sim_config, std::size_t& food_delta_local);
    bool advance_one_local_ant(WorldState& world, const SimConfig& sim_config,
                               std::size_t ant_idx, std::size_t& food_delta_local);
    void enqueue_migrated_ant(std::size_t ant_idx);
    void maybe_print_migration_debug();
    bool within_local_plus_halo_global(std::int32_t x, std::int32_t y) const;
    LocalCellCoord global_to_local_with_halo(std::int32_t x, std::int32_t y) const;
    double phen_read_global(std::int32_t x, std::int32_t y, int channel) const;
    bool mark_pheromone_global(std::int32_t x, std::int32_t y, int channel);
    bool owns_cell_global(std::int32_t x, std::int32_t y) const;
    LocalCellCoord global_to_local(std::int32_t x, std::int32_t y) const;
    void maybe_print_partition_debug();

    AntsSoA& m_ants;

    std::size_t m_global_w{0};
    std::size_t m_global_h{0};
    std::size_t m_y0{0};
    std::size_t m_y1{0}; // exclusive
    int m_rank{0};
    int m_size{1};
    int m_up_rank{-1};
    int m_down_rank{-1};
    bool m_partition_ready{false};
    bool m_partition_logged{false};
    bool m_local_ants_ready{false};
    bool m_local_grid_ready{false};
    std::uint64_t m_step_counter{0};
    std::size_t m_expected_global_ants{0};
    std::size_t m_last_out_up{0};
    std::size_t m_last_out_down{0};
    std::size_t m_last_in_up{0};
    std::size_t m_last_in_down{0};
    double m_alpha{0.7};
    std::vector<std::uint32_t> m_mark_epoch;
    std::uint32_t m_epoch{1u};
    LocalAntsSoA m_local_ants;
    LocalAntsSoA m_outbox_up;
    LocalAntsSoA m_outbox_down;
    LocalPheromoneGrid m_local_phen;
    LocalPheromoneGrid m_local_next_phen;
};
