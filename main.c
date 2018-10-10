#include <stdbool.h>
#include <stdint.h>
typedef uint32_t u32;

#include <SDL2/SDL.h>

int main(int argument_count, char ** arguments)
{
    // TODO: Handle failures.
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    // TODO: Use renderer to display pixel buffer.
    u32 * pixels = SDL_GetWindowSurface(window)->pixels;

    while (true)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) exit(0);
        }
        SDL_UpdateWindowSurface(window);
    }
}
