#ifndef _BASIC_TYPES_HPP_
#define _BASIC_TYPES_HPP_
#include <SDL2/SDL.h>

// Reuse SDL_Point as the shared integer position type across the project.
using position_t=SDL_Point;
inline bool operator == ( const position_t& pos1, const position_t& pos2 )
{
    return (pos1.x == pos2.x ) and (pos1.y == pos2.y);
}

// Use a width/height pair when passing window or grid dimensions around.
using dimension_t=std::pair<std::size_t,std::size_t>;


#endif
