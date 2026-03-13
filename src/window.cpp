#include "window.hpp"

Window::Window(const char* title, int width, int height)
{
    // Prefer an accelerated SDL window first and fall back to a simpler one if needed.
    m_window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                width,
                                height,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if ( !m_window ) {
        m_window = SDL_CreateWindow(title,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    width,
                                    height,
                                    SDL_WINDOW_SHOWN);
    }
    if (m_window) {
        // Prefer a vsynced accelerated renderer and fall back to software rendering if needed.
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if ( !m_renderer ) {
            m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_SOFTWARE);
        }
    }
}
// ====================================================================================================================
Window::~Window()
{
    if (m_window) {
        // Destroy the renderer before destroying the window that owns it.
        if ( m_renderer )
            SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
    }
}
// ====================================================================================================================
