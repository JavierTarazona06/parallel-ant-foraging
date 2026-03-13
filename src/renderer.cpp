#include <limits>
#include <algorithm>
#include "renderer.hpp"
#include "ants_soa.hpp"

Renderer::Renderer( const fractal_land& land, const pheronome& phen, 
                    const position_t& pos_nest, const position_t& pos_food,
                    const std::vector<ant>& ants )
    :   m_ref_land( land ),
        m_land( nullptr ),
        m_ref_phen( phen ),
        m_pos_nest( pos_nest ),
        m_pos_food( pos_food ),
        m_ref_ants_aos( &ants ),
        m_ref_ants_soa( nullptr )
{
    // Create the terrain texture lazily because SDL needs the final window renderer first.
}

Renderer::Renderer( const fractal_land& land, const pheronome& phen,
                    const position_t& pos_nest, const position_t& pos_food,
                    const AntsSoA& ants )
    :   m_ref_land( land ),
        m_land( nullptr ),
        m_ref_phen( phen ),
        m_pos_nest( pos_nest ),
        m_pos_food( pos_food ),
        m_ref_ants_aos( nullptr ),
        m_ref_ants_soa( &ants )
{
    // Create the terrain texture lazily because SDL needs the final window renderer first.
}
// ====================================================================================================================
Renderer::~Renderer() {
    // Release the cached terrain texture owned by the renderer wrapper.
    if ( m_land != nullptr )
        SDL_DestroyTexture( m_land );
}
// ====================================================================================================================
void Renderer::display( Window& win, std::size_t const& compteur )
{
    SDL_Renderer* renderer = SDL_GetRenderer( win.get() );
    if ( renderer == nullptr )
        return;
    
    // Build the terrain texture once and reuse it across all subsequent frames.
    if ( m_land == nullptr ) {
        // Rasterize the normalized terrain into a temporary SDL surface.
        SDL_Surface* temp_surface = SDL_CreateRGBSurface(0, m_ref_land.dimensions(), m_ref_land.dimensions(), 32,
                                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        
        double min_height{std::numeric_limits<double>::max()}, max_height{std::numeric_limits<double>::lowest()};
        for ( fractal_land::dim_t i = 0; i < m_ref_land.dimensions( ); ++i )
            for ( fractal_land::dim_t j = 0; j < m_ref_land.dimensions( ); ++j ) {
                min_height = std::min( min_height, m_ref_land( i, j ) );
                max_height = std::max( max_height, m_ref_land( i, j ) );
            }
        
        // Convert terrain heights into grayscale pixels for the cached background texture.
        for ( fractal_land::dim_t i = 0; i < m_ref_land.dimensions( ); ++i )
            for ( fractal_land::dim_t j = 0; j < m_ref_land.dimensions( ); ++j ) {
                double c = 255. * ( m_ref_land( i, j ) - min_height ) / ( max_height - min_height );
                Uint32* pixel = (Uint32*) ((Uint8*)temp_surface->pixels + j * temp_surface->pitch + i * sizeof(Uint32));
                *pixel = SDL_MapRGBA( temp_surface->format, static_cast<Uint8>(c), static_cast<Uint8>(c), static_cast<Uint8>(c), 255 );
            }
        
        // Upload the terrain surface into a reusable SDL texture.
        m_land = SDL_CreateTextureFromSurface( renderer, temp_surface );
        SDL_FreeSurface( temp_surface );
    }

    // Clear the whole SDL renderer before drawing the new frame.
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_RenderClear( renderer );

    // Draw the terrain twice so ants and pheromones can be shown side by side.
    SDL_Rect dest_rect1{0, 0, static_cast<int>(m_ref_land.dimensions()), static_cast<int>(m_ref_land.dimensions())};
    SDL_RenderCopy( renderer, m_land, nullptr, &dest_rect1 );
    SDL_Rect dest_rect2{static_cast<int>(m_ref_land.dimensions()) + 10, 0, static_cast<int>(m_ref_land.dimensions()), static_cast<int>(m_ref_land.dimensions())};
    SDL_RenderCopy( renderer, m_land, nullptr, &dest_rect2 );
    
    // Enable blending so pheromone and history overlays remain readable.
    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    // Draw ants in the left view using the active AoS or SoA container.
    win.set_pen( 0, 255, 255 );
    if ( m_ref_ants_aos != nullptr ) {
        for ( const auto& ant : *m_ref_ants_aos ) {
            const position_t& pos_ant = ant.get_position( );
            win.pset( static_cast<int>( pos_ant.x ), static_cast<int>( pos_ant.y ) );
        }
    } else if ( m_ref_ants_soa != nullptr ) {
        for ( std::size_t i = 0; i < m_ref_ants_soa->size(); ++i ) {
            win.pset( m_ref_ants_soa->x[i], m_ref_ants_soa->y[i] );
        }
    }
    
    // Draw both pheromone channels in the right view with red and green intensity.
    for ( fractal_land::dim_t i = 0; i < m_ref_land.dimensions( ); ++i )
        for ( fractal_land::dim_t j = 0; j < m_ref_land.dimensions( ); ++j ) {
            double r = std::min( 1., (double)m_ref_phen( i, j )[0] );
            double g = std::min( 1., (double)m_ref_phen( i, j )[1] );
            // Skip tiny pheromone values so the visualization stays readable.
            if ( r > 0.01 || g > 0.01 ) {
                win.set_pen( static_cast<Uint8>( r * 255 ), static_cast<Uint8>( g * 255 ), 0 );
                win.pset( static_cast<int>( i + m_ref_land.dimensions( ) + 10 ), static_cast<int>( j ) );
            }
        }
    
    // Extend and draw the food-return history curve in the bottom area of the window.
    m_curve.push_back(compteur);
    if ( m_curve.size( ) > 1 ) {
        int sz_win = win.size( ).first;
        int ydec = win.size( ).second - 1;
        // Scale the curve against the global maximum so the plot stays visually stable.
        double max_curve_val = *std::max_element( m_curve.begin(), m_curve.end() );
        double h_max_val = 256. / std::max( max_curve_val, 1.);
        double step      = double(sz_win) / (double)( m_curve.size( ) );
        
        // Draw the history curve as connected SDL lines across the full timeline.
        SDL_SetRenderDrawColor( renderer, 255, 255, 127, 255 );
        for ( std::size_t i = 0; i < m_curve.size( ) - 1; i++ ) {
            int x1 = static_cast<int>( i * step );
            int y1 = static_cast<int>( ydec - m_curve[i] * h_max_val );
            int x2 = static_cast<int>( ( i + 1 ) * step );
            int y2 = static_cast<int>( ydec - m_curve[i + 1] * h_max_val );
            SDL_RenderDrawLine( renderer, x1, y1, x2, y2 );
        }
    }
    
    // Present the fully composed frame through SDL's double-buffered renderer.
    SDL_RenderPresent( renderer );
}
