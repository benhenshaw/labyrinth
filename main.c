/*
    Labyrinth
    Benedict Henshaw, 2018
    main.c - Entry point. Where all globals, type definitions,
             and includes live.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>
#define assert(...) SDL_assert(__VA_ARGS__)
#define ENET_IMPLEMENTATION
#include "enet.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//
// TYPES
//

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  f32;
typedef double f64;

typedef struct
{
    bool pressing_up;
    bool pressing_down;
    bool pressing_left;
    bool pressing_right;
    bool entry_active;
}
Input_State;

typedef struct
{
    f32 x;
    f32 y;
    f32 walk;
    f32 walk_acceleration;
    f32 strafe;
    f32 strafe_acceleration;
    f32 angle;
    f32 speed;
    int sprite_index;
}
Player;

//
// GLOBALS
//

SDL_Window * window;
SDL_Renderer * renderer;
SDL_Texture * screen_texture;

u32 audio_device = -1;

u32 * screen_pixels;
f32 * depth_buffer;
int screen_width = 640;
int screen_height = 480;
int screen_scale = 3;

u32 * texture_pixels;
int texture_size;
int texture_count;
int texture_pitch;

u32 * sprite_pixels;
int sprite_size;
int sprite_count;
int sprite_pitch;

u32 * font_pixels;
int font_char_width;
int font_char_height;

f32 view_accuracy = 0.01f;
f32 view_distance = 32;
f32 view_angle = M_PI / 3.0f * (480.0f / 640.0f);

f32 turn_speed = 0.005f;

bool pressing_up    = false;
bool pressing_down  = false;
bool pressing_left  = false;
bool pressing_right = false;

const int max_players = 8;
char player_name[16];
ENetHost * local_host;
ENetPeer * remote_server;
int network_mode = 0;
#define NETMODE_CLIENT 1
#define NETMODE_SERVER 2
#define DEFAULT_PORT 12921

const int console_line_count = 12;
const int console_width = 128;
char console_buffer[console_line_count][console_width];

char previous_entry[console_width] = {};
char entry[console_width] = {};
int entry_index = 0;
bool entry_active = false;

int map_width  = 16;
int map_height = 16;
char map[] =
    "0000222222220000"
    "1              0"
    "1      11111   0"
    "1     0        0"
    "0     0  1110000"
    "0     3        0"
    "0   10000      0"
    "0   0   11100  0"
    "0   0   0      0"
    "0   0   1  00000"
    "0       1      0"
    "2       1      0"
    "0       0      0"
    "0 0000000      0"
    "0              0"
    "0002222222200000";

Player players[max_players];
int player_count = 8;
Player * player = players;

//
// A small handful of functions that are referenced across multiple files.
//

void draw_text(int x, int y, u32 colour, char * text, ...);
void create_network(int port);
void join_network(char * address_with_optional_port);
void set_player_name(char * name);
void toggle_fullscreen();
void send_string_over_network(char * string);

//
// LOCAL INCLUDES
//

#include "common.c"
#include "console.c"
#include "graphics.c"
#include "network.c"
#include "player.c"

int audio_callback(void * data, u8 * stream, int byte_count)
{
    // mix_audio(mixer, stream, byte_count / (sizeof(f32) * 2));
}

int main(int argument_count, char ** arguments)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        panic_exit("Could not initialise SDL2\n%s", SDL_GetError());
    }

    SDL_Rect display_bounds;
    SDL_GetDisplayBounds(0, &display_bounds);
    screen_width  = display_bounds.w / screen_scale;
    screen_height = display_bounds.h / screen_scale;

    window = SDL_CreateWindow("",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screen_width * 2, screen_height * 2, SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) panic_exit("Could not create window\n%s", SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) panic_exit("Could not create renderer\n%s", SDL_GetError());

    init_screen(screen_width, screen_height);

    load_textures("data/textures.png");
    load_sprites("data/sprites.png");
    load_font("data/font_6x12.png");

    init_network();

    SDL_WarpMouseInWindow(window, screen_width / 2, screen_height / 2);
    SDL_SetRelativeMouseMode(true);
    SDL_ShowCursor(false);

    u64 previous_counter_ticks = SDL_GetPerformanceCounter();
    f32 counter_ticks_per_second = SDL_GetPerformanceFrequency();

    set_seed(SDL_GetTicks(), SDL_GetPerformanceCounter());

    player = &players[0];
    for (int player_index = 0; player_index < player_count; ++player_index)
    {
        Player * p = players + player_index;
        p->sprite_index = player_index;
        p->speed = 1.0f;
        p->angle = random_f32_range(-PI, PI);
        p->walk = 0.0f;
        p->walk_acceleration = 0.0f;
        p->strafe = 0.0f;
        p->strafe_acceleration = 0.0f;
        randomly_spawn_player(p);
    }

    while (true)
    {
        f32 delta_time = (SDL_GetPerformanceCounter() - previous_counter_ticks) / counter_ticks_per_second;
        previous_counter_ticks = SDL_GetPerformanceCounter();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
            {
                u32 scancode = event.key.keysym.scancode;
                u32 modifiers = event.key.keysym.mod;
                bool pressed = event.key.state;
                if (!entry_active && (scancode == SDL_SCANCODE_UP || scancode == SDL_SCANCODE_W))
                {
                    pressing_up = pressed;
                }
                else if (!entry_active && (scancode == SDL_SCANCODE_DOWN || scancode == SDL_SCANCODE_S))
                {
                    pressing_down = pressed;
                }
                else if (!entry_active && (scancode == SDL_SCANCODE_LEFT || scancode == SDL_SCANCODE_A))
                {
                    pressing_left = pressed;
                }
                else if (!entry_active && (scancode == SDL_SCANCODE_RIGHT || scancode == SDL_SCANCODE_D))
                {
                    pressing_right = pressed;
                }
                else if (scancode == SDL_SCANCODE_F11 && pressed)
                {
                    toggle_fullscreen();
                }
                else if (scancode == SDL_SCANCODE_RETURN && pressed)
                {
                    if (entry_active)
                    {
                        submit_entry();
                    }
                    else
                    {
                        entry_active = true;
                        pressing_up = pressing_down = pressing_left = pressing_right = false;
                    }
                }
                else if (scancode == SDL_SCANCODE_BACKSPACE && pressed)
                {
                    if (modifiers & KMOD_CTRL)
                    {
                        clear_console();
                    }
                    else
                    {
                        remove_entry_character();
                    }
                }
                else if (!entry_active && scancode == SDL_SCANCODE_SLASH && pressed)
                {
                    entry_active = true;
                    pressing_up = pressing_down = pressing_left = pressing_right = false;
                }
                else if (entry_active && scancode == SDL_SCANCODE_UP)
                {
                    load_previous_entry();
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                f32 delta = event.motion.xrel * turn_speed;
                update_player_angle(delta);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                shoot();
            }
            else if (event.type == SDL_TEXTINPUT)
            {
                if (entry_active && event.text.text[1] == '\0')
                {
                    push_entry_character(event.text.text[0]);
                }
            }
            else if (event.type == SDL_QUIT)
            {
                exit(0);
            }
        }

        handle_network();

        player->walk_acceleration   = 0.0f;
        player->strafe_acceleration = 0.0f;
        f32 speed = player->speed * delta_time;
        if (pressing_up)    player->walk_acceleration += speed;
        if (pressing_down)  player->walk_acceleration -= speed;
        if (pressing_left)  player->strafe_acceleration -= speed;
        if (pressing_right) player->strafe_acceleration += speed;

        for (int i = 0; i < player_count; ++i)
        {
            update_player_position(players + i, delta_time);
        }

        render_background();
        render_player_view(player);
        render_players();

        for (int i = 0; i < screen_width * screen_height; ++i)
        {
            u32 random_colour = random_int_range(-5, 5);
            random_colour = rgba(random_colour, random_colour, random_colour, 255);
            screen_pixels[i] += random_colour;
        }

        draw_crosshair();
        draw_console();
        display_screen();
    }
}
