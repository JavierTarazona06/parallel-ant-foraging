#pragma once

#include <cstddef>
#include <vector>

class LocalPheromoneGrid
{
public:
    using value_type = double;

    // Store one local pheromone subgrid with a one-cell halo on every side.
    LocalPheromoneGrid() = default;
    LocalPheromoneGrid(std::size_t global_w, std::size_t local_h, value_type halo_value = -1.0);

    void reset(std::size_t global_w, std::size_t local_h, value_type halo_value = -1.0);

    std::size_t global_width() const { return m_global_w; }
    std::size_t local_height() const { return m_local_h; }
    std::size_t stride() const { return m_stride; }
    std::size_t row_count() const { return m_rows; }

    value_type& v1(std::size_t lx, std::size_t ly);
    const value_type& v1(std::size_t lx, std::size_t ly) const;

    value_type& v2(std::size_t lx, std::size_t ly);
    const value_type& v2(std::size_t lx, std::size_t ly) const;

    // Expose contiguous row pointers for halo exchange with neighboring ranks.
    value_type* v1_row_ptr(std::size_t ly);
    const value_type* v1_row_ptr(std::size_t ly) const;

    value_type* v2_row_ptr(std::size_t ly);
    const value_type* v2_row_ptr(std::size_t ly) const;

    value_type* v1_data() { return m_v1.data(); }
    const value_type* v1_data() const { return m_v1.data(); }

    value_type* v2_data() { return m_v2.data(); }
    const value_type* v2_data() const { return m_v2.data(); }

    std::size_t element_count() const { return m_v1.size(); }

private:
    std::size_t index(std::size_t lx, std::size_t ly) const;

    std::size_t m_global_w{0};
    std::size_t m_local_h{0};
    std::size_t m_stride{0};
    std::size_t m_rows{0};

    std::vector<value_type> m_v1;
    std::vector<value_type> m_v2;
};
