#ifndef _PHERONOME_HPP_
#define _PHERONOME_HPP_
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>
#include "basic_types.hpp"

/**
 * @brief Carte des phéronomes
 * @details Gère une carte des phéronomes avec leurs mis à jour ( dont l'évaporation )
 *
 */
class pheronome {
public:
    using size_t      = unsigned long;
    using pheronome_t = std::array< double, 2 >;

    struct cell_ref {
        double* v1{nullptr};
        double* v2{nullptr};

        double& operator[](std::size_t idx)
        {
            assert(idx < 2);
            return (idx == 0) ? *v1 : *v2;
        }

        const double& operator[](std::size_t idx) const
        {
            assert(idx < 2);
            return (idx == 0) ? *v1 : *v2;
        }
    };

    struct cell_const_ref {
        const double* v1{nullptr};
        const double* v2{nullptr};

        const double& operator[](std::size_t idx) const
        {
            assert(idx < 2);
            return (idx == 0) ? *v1 : *v2;
        }
    };

    /**
     * @brief Construit une carte initiale des phéronomes
     * @details La carte des phéronomes est initialisées à zéro ( neutre )
     *          sauf pour les bords qui sont marqués comme indésirables
     *
     * @param dim Nombre de cellule dans chaque direction
     * @param alpha Paramètre de bruit
     * @param beta Paramêtre d'évaporation
     */
    pheronome( size_t dim, const position_t& pos_food, const position_t& pos_nest,
               double alpha = 0.7, double beta = 0.9999 )
        : m_dim( dim ),
          m_stride( dim + 2 ),
          m_alpha(alpha), m_beta(beta),
          m_map_v1( m_stride * m_stride, 0.0 ),
          m_map_v2( m_stride * m_stride, 0.0 ),
          m_buffer_v1( m_stride * m_stride, 0.0 ),
          m_buffer_v2( m_stride * m_stride, 0.0 ),
          m_pos_nest( pos_nest ),
          m_pos_food( pos_food ) 
          {
        m_map_v1[index(pos_food)] = 1.;
        m_map_v2[index(pos_nest)] = 1.;
        cl_update( );
        m_buffer_v1 = m_map_v1;
        m_buffer_v2 = m_map_v2;
    }
    pheronome( const pheronome& ) = delete;
    pheronome( pheronome&& )      = delete;
    ~pheronome( )                 = default;

    cell_ref operator( )( size_t i, size_t j ) {
        const size_t idx = flat_index(i, j);
        return cell_ref{&m_map_v1[idx], &m_map_v2[idx]};
    }

    cell_const_ref operator( )( size_t i, size_t j ) const {
        const size_t idx = flat_index(i, j);
        return cell_const_ref{&m_map_v1[idx], &m_map_v2[idx]};
    }

    cell_ref operator[] ( const position_t& pos ) {
      const size_t idx = index(pos);
      return cell_ref{&m_map_v1[idx], &m_map_v2[idx]};
    }

    cell_const_ref operator[] ( const position_t& pos ) const {
      const size_t idx = index(pos);
      return cell_const_ref{&m_map_v1[idx], &m_map_v2[idx]};
    }

    // Contiguous channels for MPI reductions (V1 / V2).
    std::vector<double>& v1_buffer() { return m_map_v1; }
    const std::vector<double>& v1_buffer() const { return m_map_v1; }
    std::vector<double>& v2_buffer() { return m_map_v2; }
    const std::vector<double>& v2_buffer() const { return m_map_v2; }

    double* v1_data() { return m_map_v1.data(); }
    const double* v1_data() const { return m_map_v1.data(); }
    std::size_t v1_size() const { return m_map_v1.size(); }

    double* v2_data() { return m_map_v2.data(); }
    const double* v2_data() const { return m_map_v2.data(); }
    std::size_t v2_size() const { return m_map_v2.size(); }

    std::size_t cell_count() const { return m_map_v1.size(); }

    void do_evaporation( ) {
        for ( std::size_t i = 1; i <= m_dim; ++i )
            for ( std::size_t j = 1; j <= m_dim; ++j ) {
                const std::size_t idx = i * m_stride + j;
                m_buffer_v1[idx] *= m_beta;
                m_buffer_v2[idx] *= m_beta;
            }
    }

    void mark_pheronome( std::int32_t x, std::int32_t y ) {
      assert( x >= 0 );
      assert( y >= 0 );
      std::size_t i = static_cast<std::size_t>(x);
      std::size_t j = static_cast<std::size_t>(y);
        assert( i < m_dim );
        assert( j < m_dim );
        pheronome&         phen        = *this;
        double             v1_left     = std::max( phen( i - 1, j )[0], 0. );
        double             v2_left     = std::max( phen( i - 1, j )[1], 0. );
        double             v1_right    = std::max( phen( i + 1, j )[0], 0. );
        double             v2_right    = std::max( phen( i + 1, j )[1], 0. );
        double             v1_upper    = std::max( phen( i, j - 1 )[0], 0. );
        double             v2_upper    = std::max( phen( i, j - 1 )[1], 0. );
        double             v1_bottom   = std::max( phen( i, j + 1 )[0], 0. );
        double             v2_bottom   = std::max( phen( i, j + 1 )[1], 0. );
        const std::size_t idx = flat_index(i, j);
        m_buffer_v1[idx] =
            m_alpha * std::max( {v1_left, v1_right, v1_upper, v1_bottom} ) +
            ( 1 - m_alpha ) * 0.25 * ( v1_left + v1_right + v1_upper + v1_bottom );
        m_buffer_v2[idx] =
            m_alpha * std::max( {v2_left, v2_right, v2_upper, v2_bottom} ) +
            ( 1 - m_alpha ) * 0.25 * ( v2_left + v2_right + v2_upper + v2_bottom );
    }

    void mark_pheronome( const position_t& pos ) {
        mark_pheronome(pos.x, pos.y);
    }

    void update( ) {
        m_map_v1.swap( m_buffer_v1 );
        m_map_v2.swap( m_buffer_v2 );
        cl_update( );
        m_map_v1[( m_pos_food.x + 1 ) * m_stride + m_pos_food.y + 1] = 1;
        m_map_v2[( m_pos_nest.x + 1 ) * m_stride + m_pos_nest.y + 1] = 1;
    }

private:
    size_t flat_index( size_t i, size_t j ) const
    {
      return ( i + 1 ) * m_stride + ( j + 1 );
    }

    size_t index( const position_t& pos ) const
    {
      return (pos.x+1)*m_stride + pos.y + 1;
    }
    /**
     * @brief Mets à jour les conditions limites sur les cellules fantômes
     * @details Mets à jour les conditions limites sur les cellules fantômes :
     *     pour l'instant, on se contente simplement de mettre ces cellules avec
     *     des valeurs à -1 pour être sûr que les fourmis évitent ces cellules
     */
    void cl_update( ) {
        // On mets tous les bords à -1 pour les marquer comme indésirables :
        for ( unsigned long j = 0; j < m_stride; ++j ) {
            m_map_v1[j]                            = -1.;
            m_map_v2[j]                            = -1.;
            m_map_v1[j + m_stride * ( m_dim + 1 )] = -1.;
            m_map_v2[j + m_stride * ( m_dim + 1 )] = -1.;
            m_map_v1[j * m_stride]                 = -1.;
            m_map_v2[j * m_stride]                 = -1.;
            m_map_v1[j * m_stride + m_dim + 1]     = -1.;
            m_map_v2[j * m_stride + m_dim + 1]     = -1.;
        }
    }
    unsigned long              m_dim, m_stride;
    double                     m_alpha, m_beta;
    std::vector<double> m_map_v1, m_map_v2;
    std::vector<double> m_buffer_v1, m_buffer_v2;
    position_t m_pos_nest, m_pos_food;
};

#endif
