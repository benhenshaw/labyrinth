/*
    Labyrinth
    Benedict Henshaw, 2018
    graphics.c - Rendering of all things, including player's view, text, and sprites.
*/

void init_screen(int width, int height)
{
    screen_width = width;
    screen_height = height;

    screen_pixels = realloc(screen_pixels, screen_width * screen_height * sizeof(*screen_pixels));
    assert(screen_pixels);
    depth_buffer = realloc(depth_buffer, screen_width * sizeof(*depth_buffer));;
    assert(depth_buffer);

    view_angle = PI / (3.0f * ((f32)screen_height/(f32)screen_width));

    if (screen_texture) SDL_DestroyTexture(screen_texture);
    screen_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
        screen_width, screen_height);
    if (!screen_texture)
    {
        panic_exit("Could not create screen texture.\n%s\n", SDL_GetError());
    }

    SDL_RenderSetLogicalSize(renderer, screen_width, screen_height);
    SDL_RenderSetIntegerScale(renderer, true);
}

void toggle_fullscreen()
{
    int flag = SDL_GetWindowFlags(window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP;
    flag &= SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(window, flag);
}

void display_screen()
{
    SDL_Delay(1);
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(screen_texture, NULL, screen_pixels, screen_width * sizeof(*screen_pixels));
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

//
// Asset loading. (TODO: Move?)
//

void load_textures(char * file_name)
{
    if (texture_pixels) free(texture_pixels);

    int width, height;
    texture_pixels = (u32 *)stbi_load(file_name, &width, &height, NULL, 4);
    if (!texture_pixels)
    {
        panic_exit("Could not load textures.\n");
    }

    texture_pitch = width;
    texture_size = height;
    texture_count = width / height;
    if (width != texture_size * texture_count)
    {
        panic_exit("Incorrect texture size.\n");
    }
}

void load_sprites(char * file_name)
{
    if (sprite_pixels) free(sprite_pixels);

    int width, height;
    sprite_pixels = (u32 *)stbi_load(file_name, &width, &height, NULL, 4);
    if (!sprite_pixels)
    {
        panic_exit("Could not load sprites.\n");
    }

    sprite_pitch = width;
    sprite_size = height;
    sprite_count = width / height;
    if (width != sprite_size * sprite_count)
    {
        panic_exit("Incorrect texture size.\n");
    }
}

void load_font(char * file_name)
{
    font_pixels = (u32 *)stbi_load(file_name, &font_char_width, &font_char_height, NULL, 4);
    if (!font_pixels) panic_exit("Could not load font bitmap.\n");
    font_char_width /= ('~' - ' ');
}

//
// Basic graphics manipulation and primatives.
//

u32 rgba(u8 r, u8 g, u8 b, u8 a)
{
    return (r << 24u) | (g << 16u) | (b << 8u) | a;
}

u32 get_texture_pixel(int texture_index, int x, int y)
{
    if (texture_index >= 0 &&
        texture_index < texture_count &&
        x >= 0 && x < texture_size &&
        y >= 0 && y < texture_size)
    {
        x += texture_size * texture_index;
        return texture_pixels[x + y * texture_pitch];
    }
    return 0;
}

void set_screen_pixel(int x, int y, u32 colour)
{
    if (x >= 0 && x < screen_width && y >= 0 && y < screen_height)
    {
        screen_pixels[x + y * screen_width] = colour;
    }
}

void draw_box(int ax, int ay, int bx, int by, u32 colour)
{
    for (int x = ax; x <= bx; ++x) set_screen_pixel(x, ay, colour);
    for (int x = ax; x <= bx; ++x) set_screen_pixel(x, by, colour);
    for (int y = ay; y <= by; ++y) set_screen_pixel(ax, y, colour);
    for (int y = ay; y <= by; ++y) set_screen_pixel(bx, y, colour);
}

void draw_line(int ax, int ay, int bx, int by, u32 colour)
{
    int delta_x = abs(bx - ax);
    int delta_y = abs(by - ay);
    int step_x  = ax < bx ? 1 : -1;
    int step_y  = ay < by ? 1 : -1;
    int error   = (delta_x > delta_y ? delta_x : -delta_y) / 2;

    // Stop drawing if pixel is off screen or we have reached the end of the line.
    while (!(ax == bx && ay == by))
    {
        set_screen_pixel(ax, ay, colour);
        int e = error;
        if (e > -delta_x) error -= delta_y, ax += step_x;
        if (e <  delta_y) error += delta_x, ay += step_y;
    }
}

//
// Text rendering.
//

void draw_text(int x, int y, u32 colour, char * text, ...)
{
    #define TEXT_MAX 128
    char formatted_text[TEXT_MAX];
    va_list args;
    va_start(args, text);
    int char_count = vsnprintf(formatted_text, TEXT_MAX, text, args);
    char_count = min(char_count, TEXT_MAX);
    va_end(args);
    // 95 is the number of drawable characters including the single space.
    int total_width = 95 * font_char_width;
    int x_offset = 0;
    int y_offset = 0;
    for (int c = 0; c < char_count; ++c)
    {
        if (formatted_text[c] >= ' ' && formatted_text[c] <= '~')
        {
            int text_start_x = font_char_width * (formatted_text[c] - ' ');
            int max_x = min(x + font_char_width, screen_width);
            int max_y = min(y + font_char_height, screen_height);

            for (int sy = y, iy = 0; sy < max_y; ++sy, ++iy)
            {
                for (int sx = x, ix = text_start_x; sx < max_x; ++sx, ++ix)
                {
                    if (font_pixels[ix + iy * total_width])
                    {
                        set_screen_pixel(sx + x_offset, sy + y_offset + 1, 0);
                    }
                }
            }

            for (int sy = y, iy = 0; sy < max_y; ++sy, ++iy)
            {
                for (int sx = x, ix = text_start_x; sx < max_x; ++sx, ++ix)
                {
                    if (font_pixels[ix + iy * total_width])
                    {
                        set_screen_pixel(sx + x_offset, sy + y_offset, colour);
                    }
                }
            }

            x_offset += font_char_width;
        }
        else if (formatted_text[c] == '\n')
        {
            y_offset += font_char_height;
            x_offset = 0;
        }
        else if (formatted_text[c] == '\t')
        {
            int tab_size = 4;
            x_offset +=  ((font_char_width * tab_size) - 1);
            x_offset &= ~((font_char_width * tab_size) - 1);
        }
    }
}

//
// Player view rendering.
//

void render_background()
{
    // Draw ceiling.
    int pixel_count = screen_width * (screen_height / 2);
    for (int i = 0; i < pixel_count; ++i)
    {
        screen_pixels[i] = 0xffB6B6B6;
    }
    // Draw floor.
    for (int i = pixel_count; i < screen_width * screen_height; ++i)
    {
        screen_pixels[i] = 0xff3C3C3C;
    }
}

int get_screen_x(f32 x, f32 y, Player * player)
{
    // Calculate angle between player and point.
    f32 angle = atan2f(y - player->y, x - player->x);
    // Rotate further by the players view angle.
    angle -= player->angle;
    // Clamp angle to a resonable range.
    angle -= angle >  PI ? TWO_PI : 0.0f;
    angle += angle < -PI ? TWO_PI : 0.0f;
    // Calculate screen x coordinate (inverse of the ray-cast found in render_player_view)
    return (angle * screen_width) / view_angle + screen_width / 2;
}

void render_sprite(f32 x, f32 y, u32 * pixels, int size, int pitch, Player * player)
{
    f32 distance = dist2(player->x, player->y, x, y);
    f32 scale = (screen_height / size) / distance;
    int scaled_width  = size * scale;
    int scaled_height = size * scale;
    int screen_x = get_screen_x(x, y, player) - scaled_width / 2;
    int screen_y = screen_height / 2 - scaled_height / 2;
    for (int py = 0; py < scaled_height; ++py)
    {
        for (int px = 0; px < scaled_width; ++px)
        {
            int tx = ((f32)px / (f32)scaled_width)  * size;
            int ty = ((f32)py / (f32)scaled_height) * size;
            u32 pixel = pixels[tx + ty * pitch];
            if (distance < depth_buffer[screen_x + px] && pixel)
            {
                set_screen_pixel(screen_x + px, screen_y + py, pixel);
            }
        }
    }
}

void render_player_view(Player * player)
{
    // Draw view for each column of pixels.
    for (int screen_x = 0; screen_x < screen_width; ++screen_x)
    {
        // Find the angle from the player view that corresponds to this pixel column.
        f32 ca = (1.0f - screen_x / (f32)screen_width) * (player->angle - view_angle / 2.0f)
                       + screen_x / (f32)screen_width  * (player->angle + view_angle / 2.0f);

        f32 cca = cosf(ca);
        f32 sca = sinf(ca);

        // Incrementally step forward along this angle, away from the player.
        for (f32 distance = MIN_DISTANCE_FROM_WALL;
            distance < view_distance;
            distance += view_accuracy)
        {
            // Find the world coordinates of this stepping point.
            f32 cx = player->x + cca * distance;
            f32 cy = player->y + sca * distance;

            // If these coordinates are inside a tile that is not empty.
            int tile_index = (int)cx + (int)cy * map_width;
            if (map[tile_index] != ' ')
            {
                // Calculate the height of the wall using its distance.
                int h = screen_height / distance;

                // Get the x coordinate of the pixel in the texture to draw. The fractional portion
                // of the current ray stepping point coordinates are used. Since the greater of the
                // two fractional portions is used to determine which case (x or y axis-aligned
                // wall), we must avoid cases where inaccuracy causes 1.0 to be calculated as 0.999
                // and that fractional portion is taken as larger, when it should be near zero.
                // Adding view_accuracy will correct those .999 values to be past the next integer,
                // thus correctly making the fractional portion of that number some small value.
                int tx = fmaxf((cx - (int)(cx + view_accuracy)),
                               (cy - (int)(cy + view_accuracy))) * texture_size;

                // Get the texture of the tile.
                int texture_id = map[tile_index] - '0';

                // Draw the pixel column.
                for (int ty = 0; ty < h; ++ty)
                {
                    u32 colour = get_texture_pixel(texture_id, tx, (ty * texture_size) / h);
                    set_screen_pixel(screen_x, ty + (screen_height - h) / 2, colour);
                }

                // We hit a solid object, so stop casting this ray and save the depth to the buffer.
                depth_buffer[screen_x] = distance;
                break;
            }
        }
    }
}

void render_players()
{
    // TODO: Make this faster if we want to support more players.
    bool rendered[max_players] = {false};
    for (int i = 0; i < player_count; ++i)
    {
        // No players closer than this value will be drawn.
        f32 furthest_distance = 0.2f;
        int furthest_index = -1;
        for (int player_index = 0; player_index < player_count; ++player_index)
        {
            Player * p = players + player_index;
            if (p != player && !rendered[player_index])
            {
                f32 distance = dist2(p->x, p->y, player->x, player->y);
                if (distance > furthest_distance)
                {
                    furthest_distance = distance;
                    furthest_index = player_index;
                }
            }
        }
        if (furthest_index == -1) break;
        Player * furthest_player = players + furthest_index;
        render_sprite(furthest_player->x, furthest_player->y,
            sprite_pixels + (furthest_player->sprite_index * sprite_size),
            sprite_size, sprite_pitch,
            player);
        rendered[furthest_index] = true;
    }
}

void draw_crosshair()
{
    // TODO: Crosshair bitmap?
    int cx = screen_width / 2;
    int cy = screen_height / 2;
    int cross_hair_size = 3;
    for (int y = cy - cross_hair_size + 1; y < cy + cross_hair_size; ++y)
    {
        screen_pixels[cx + y * screen_width] = ~0;
    }
    for (int x = cx - cross_hair_size + 1; x < cx + cross_hair_size; ++x)
    {
        screen_pixels[x + cy * screen_width] = ~0;
    }
}
