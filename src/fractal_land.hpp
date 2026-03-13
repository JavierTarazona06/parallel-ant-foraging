#ifndef _FRACTAL_LAND_HPP_
# define _FRACTAL_LAND_HPP_
// Generate the terrain cost map used as movement effort for each grid cell.
# include <vector>
# include <utility>

// Build a recursive fractal height field that becomes the movement-cost grid.
class fractal_land
{
public:
    using container=std::vector<double>;
    using dim_t=unsigned long;
    fractal_land( const dim_t& log_size, unsigned long nbSeeds, double deviation, int seed = 0 );
    fractal_land( const fractal_land& ) = delete;
    fractal_land( fractal_land&& land ) = default;
    ~fractal_land() = default;

    double operator () ( unsigned long i, unsigned long j ) const {
        return m_altitude[i+j*m_dimensions];
    }
    double& operator () ( unsigned long i, unsigned long j ) {
        return m_altitude[i+j*m_dimensions];
    }
    dim_t dimensions() const { return m_dimensions; }
    double* data() { return m_altitude.data(); }
    const double* data() const { return m_altitude.data(); }

private:
    // Refine one fractal subgrid from its corner seeds and local deviation.
    void compute_subgrid( int log_subgrid_dim, int iB, int jB, double deviation, std::size_t seed );
    dim_t m_dimensions;
    container m_altitude;
};
#endif
