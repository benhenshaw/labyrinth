/*
    Labyrinth
    By Benedict Henshaw, 2018
    main.c
*/

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>

typedef uint32_t u32;

SDL_Window * window;
u32 * screen_pixels;
int screen_width = 640;
int screen_height = 480;

void panic_exit(char * message, ...)
{
    va_list args;
    va_start(args, message);
    char buffer[256];
    vsnprintf(buffer, 256, message, args);
    va_end(args);
    puts(buffer);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", buffer, NULL);
    exit(1);
}

int main(int argument_count, char ** arguments)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        panic_exit("Could not init SDL2.\n%s", SDL_GetError());
    }

    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, 0);
    if (!window)
    {
        panic_exit("Could not create window.\n%s", SDL_GetError());
    }

    // TODO: Use renderer to display pixel buffer.
    screen_pixels = SDL_GetWindowSurface(window)->pixels;

    while (true)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT) exit(0);
        }

        for (int i = 0; i < screen_width * screen_height; ++i)
        {
            screen_pixels[i] = 0xffff00;
        }

        SDL_UpdateWindowSurface(window);
    }
}
