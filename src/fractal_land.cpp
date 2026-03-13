# include "fractal_land.hpp"
# include "rand_generator.hpp"

void fractal_land::compute_subgrid( int log_subgrid_dim, int iB, int jB, double deviation,
                                    std::size_t seed )
{
    // Generate bounded pseudo-random offsets to refine one fractal subgrid.
    RandomGenerator gen( seed, -deviation, deviation );

    fractal_land& cur_land = *this;
    unsigned long dim_ss_grid = 1UL<<(log_subgrid_dim);
    unsigned long iBeg = iB*dim_ss_grid;
    unsigned long jBeg = jB*dim_ss_grid;
    int mid_ind = dim_ss_grid/2;
    int i_mid = iBeg+mid_ind, j_mid = jBeg+mid_ind;
    int iEnd  = iBeg + dim_ss_grid, jEnd = jBeg + dim_ss_grid;
    cur_land(i_mid,jBeg)  = 0.5* (cur_land(iBeg,jBeg)+cur_land(iEnd,jBeg))+mid_ind*gen(i_mid,jBeg);
    cur_land(iBeg,j_mid)  = 0.5* (cur_land(iBeg,jBeg)+cur_land(iBeg,jEnd))+mid_ind*gen(iBeg,j_mid);
    cur_land(i_mid,jEnd)  = 0.5* (cur_land(iBeg,jEnd)+cur_land(iEnd,jEnd))+mid_ind*gen(i_mid,jEnd);
    cur_land(iEnd,j_mid)  = 0.5* (cur_land(iEnd,jBeg)+cur_land(iEnd,jEnd))+mid_ind*gen(iEnd,j_mid);
    cur_land(i_mid,j_mid) = 0.25*(cur_land(i_mid,jBeg)+cur_land(iBeg,j_mid)+cur_land(i_mid,jEnd)+cur_land(iEnd,j_mid))+ mid_ind*gen(i_mid,j_mid);
}


fractal_land::fractal_land( const dim_t& ln2_dim, unsigned long nbSeeds, double deviation, int seed ) :
    m_dimensions(0), m_altitude()
{
    // Compute the initial coarse-grid size from the requested log2 dimension.
    unsigned long dim_ss_grid = 1UL<<(ln2_dim);
    m_dimensions = nbSeeds*dim_ss_grid+1;
    container(m_dimensions*m_dimensions).swap(m_altitude);

    // Seed the terrain generator once so the same seed rebuilds the same landscape.
    RandomGenerator gen(seed, 0., dim_ss_grid*deviation);

    fractal_land& cur_land = *this;
    // Initialize the coarse grid corners before recursive refinement begins.
    for ( dim_t i = 0; i < m_dimensions; i += dim_ss_grid )
        for ( dim_t j = 0; j < m_dimensions; j += dim_ss_grid )
            cur_land(i,j) = gen(i,j);
    // Recursively refine each subgrid until the target terrain resolution is reached.
    dim_t ldim = ln2_dim;
    while (ldim > 1)
    {
        ldim -= 1;
        dim_ss_grid /= 2;
        nbSeeds *= 2;
        for ( unsigned long iB = 0; iB < nbSeeds; ++iB )
            for ( unsigned long jB = 0; jB < nbSeeds; ++jB )
               compute_subgrid( ldim, iB, jB, deviation, seed);                
    }
}
