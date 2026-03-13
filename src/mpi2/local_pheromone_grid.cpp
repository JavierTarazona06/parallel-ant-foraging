#include "local_pheromone_grid.hpp"

#include <cassert>

LocalPheromoneGrid::LocalPheromoneGrid(std::size_t global_w, std::size_t local_h, value_type halo_value)
{
    // Build the local subgrid immediately so MPI2 helpers can fill it right away.
    reset(global_w, local_h, halo_value);
}

void LocalPheromoneGrid::reset(std::size_t global_w, std::size_t local_h, value_type halo_value)
{
    // Allocate a local grid with a one-cell halo around the owned interior region.
    m_global_w = global_w;
    m_local_h = local_h;
    m_stride = m_global_w + 2u;
    m_rows = m_local_h + 2u;

    const std::size_t n = m_stride * m_rows;
    m_v1.assign(n, halo_value);
    m_v2.assign(n, halo_value);
}

LocalPheromoneGrid::value_type& LocalPheromoneGrid::v1(std::size_t lx, std::size_t ly)
{
    return m_v1[index(lx, ly)];
}

const LocalPheromoneGrid::value_type& LocalPheromoneGrid::v1(std::size_t lx, std::size_t ly) const
{
    return m_v1[index(lx, ly)];
}

LocalPheromoneGrid::value_type& LocalPheromoneGrid::v2(std::size_t lx, std::size_t ly)
{
    return m_v2[index(lx, ly)];
}

const LocalPheromoneGrid::value_type& LocalPheromoneGrid::v2(std::size_t lx, std::size_t ly) const
{
    return m_v2[index(lx, ly)];
}

LocalPheromoneGrid::value_type* LocalPheromoneGrid::v1_row_ptr(std::size_t ly)
{
    // Return a contiguous row pointer so halo exchange can send or receive one full row.
    assert(ly < m_rows);
    return m_v1.data() + ly * m_stride;
}

const LocalPheromoneGrid::value_type* LocalPheromoneGrid::v1_row_ptr(std::size_t ly) const
{
    assert(ly < m_rows);
    return m_v1.data() + ly * m_stride;
}

LocalPheromoneGrid::value_type* LocalPheromoneGrid::v2_row_ptr(std::size_t ly)
{
    // Return a contiguous row pointer so halo exchange can send or receive one full row.
    assert(ly < m_rows);
    return m_v2.data() + ly * m_stride;
}

const LocalPheromoneGrid::value_type* LocalPheromoneGrid::v2_row_ptr(std::size_t ly) const
{
    assert(ly < m_rows);
    return m_v2.data() + ly * m_stride;
}

std::size_t LocalPheromoneGrid::index(std::size_t lx, std::size_t ly) const
{
    // Flatten local coordinates into the contiguous storage used for both channels.
    assert(lx < m_stride);
    assert(ly < m_rows);
    return ly * m_stride + lx;
}
