#pragma once
#include <SDL2/SDL.h>

class Window
{
public:
    Window(const char* title, int width, int height);
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    ~Window();

    Window& operator = (const Window&) = delete;
    Window& operator = (Window&&) = delete;

    SDL_Window* get() { return m_window; }
    bool is_ready() const { return m_window != nullptr && m_renderer != nullptr; }
    SDL_Surface* getSurface() { return SDL_GetWindowSurface(m_window); }

    void set_pen( Uint8 r, Uint8 g, Uint8 b ) {
        if ( m_renderer )
            SDL_SetRenderDrawColor( m_renderer, r, g, b, 255 );
    }

    void pset( int x, int y ) {
        if ( m_renderer )
            SDL_RenderDrawPoint( m_renderer, x, y );
    }

    void clear() {
        if ( m_renderer )
            SDL_RenderClear( m_renderer );
    }

    void draw( SDL_Point const* points, int count ) {
        if ( m_renderer )
            SDL_RenderDrawPoints( m_renderer, points, count );
    }

    void line( int x1, int y1, int x2, int y2 ) {
        if ( m_renderer )
            SDL_RenderDrawLine( m_renderer, x1, y1, x2, y2 );
    }

    void blit() {
        if ( m_renderer )
            SDL_RenderPresent( m_renderer );
    }

    std::pair<int, int> size() {
        int w, h;
        SDL_GetWindowSize( m_window, &w, &h );
        return { w, h };
    }

private:
    SDL_Window* m_window{ nullptr };
    SDL_Renderer* m_renderer{ nullptr };
};
