#pragma once

#include <cstdint>
#include <cstddef>

#include "backend.hpp"

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
    struct LocalCellCoord {
        std::int32_t lx{0};
        std::int32_t ly{0};
    };

    void initialize_partition_if_needed(const WorldState& world);
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
};
