#pragma once
#include "fractal_land.hpp"
#include "ant.hpp"
#include "pheronome.hpp"
#include "window.hpp"

struct AntsSoA;

class Renderer
{
public:
    // Bind the renderer to the AoS ant container used by the selected backend.
    Renderer(  const fractal_land& land, const pheronome& phen, 
               const position_t& pos_nest, const position_t& pos_food,
               const std::vector<ant>& ants );
    // Bind the renderer to the SoA ant container used by the selected backend.
    Renderer(  const fractal_land& land, const pheronome& phen,
               const position_t& pos_nest, const position_t& pos_food,
               const AntsSoA& ants );

    Renderer(const Renderer& ) = delete;
    ~Renderer();

    // Draw terrain, ants, pheromones, and food history into the active SDL window.
    void display( Window& win, std::size_t const& compteur );
private:
    // Keep references to simulation state so rendering always reflects the latest iteration.
    fractal_land const& m_ref_land;
    SDL_Texture* m_land{ nullptr }; 
    const pheronome& m_ref_phen;
    const position_t& m_pos_nest;
    const position_t& m_pos_food;
    const std::vector<ant>* m_ref_ants_aos{ nullptr };
    const AntsSoA* m_ref_ants_soa{ nullptr };
    std::vector<std::size_t> m_curve;    
};
