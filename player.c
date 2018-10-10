//
// player.c
//

void update_player_angle(f32 angle_delta)
{
    player->angle += angle_delta;
    player->angle -= player->angle >  PI ? TWO_PI : 0.0f;
    player->angle += player->angle < -PI ? TWO_PI : 0.0f;
}

void update_player_position(Player * player, f32 delta_time)
{
    // Clamp speed when moving diagonally.
    f32 max_speed = (player->walk_acceleration   != 0.0f &&
                     player->strafe_acceleration != 0.0f)
                        ? 0.2f : 0.1414214f;
    // TODO: Framerate independent acceleration and damping.
    f32 damping = 0.85f;
    player->walk *= damping;
    player->walk += player->walk_acceleration;
    player->walk = clamp(-max_speed, player->walk, max_speed);

    player->strafe *= damping;
    player->strafe += player->strafe_acceleration;
    player->strafe = clamp(-max_speed, player->strafe, max_speed);

    int current_tile_x = player->x;
    int current_tile_y = player->y;

    // How close to a wall the player can get before a collision occurs.
    f32 radius = 0.1f;

    // Calculating x and y movement separately allows one to occur while the
    // other is blocked. This allows players to slide smoothly against walls.

    f32 new_x = player->x
        + player->strafe * cosf(player->angle + HALF_PI)
        + player->walk   * cosf(player->angle);

    int new_tile_x = 0;
    if (new_x > player->x) new_tile_x = new_x + radius;
    else                  new_tile_x = new_x - radius;

    if (map[new_tile_x + current_tile_y * map_width] == ' ')
    {
        player->x = new_x;
    }
    else
    {
        // Collision has occurred, move to the nearest non-colliding place.
        if (new_x > player->x) player->x = roundf(new_x) - radius;
        else                  player->x = roundf(new_x) + radius;
    }

    f32 new_y = player->y
        + player->strafe * sinf(player->angle + HALF_PI)
        + player->walk   * sinf(player->angle);

    int new_tile_y = 0;
    if (new_y > player->y) new_tile_y = new_y + radius;
    else                  new_tile_y = new_y - radius;

    if (map[current_tile_x + new_tile_y * map_width] == ' ')
    {
        player->y = new_y;
    }
    else
    {
        // Collision has occurred, move to the nearest non-colliding place.
        if (new_y > player->y) player->y = roundf(new_y) - radius;
        else                  player->y = roundf(new_y) + radius;
    }
}

void randomly_spawn_player(Player * p)
{
    while (true)
    {
        int x = random_int_range(0, map_width);
        int y = random_int_range(0, map_height);
        if (map[x + y * map_width] == ' ')
        {
            p->x = x + 0.5f;
            p->y = y + 0.5f;
            p->angle = random_f32_range(-PI, PI);
            break;
        }
    }
}

void shoot()
{
    int index_of_player_to_kill = -1;
    f32 closest_distance = FLT_MAX;
    for (int player_index = 0; player_index < player_count; ++player_index)
    {
        Player * p = players + player_index;
        if (p != player)
        {
            f32 distance = dist2(player->x, player->y,
                p->x, p->y);
            f32 scale = (screen_height / sprite_size) / distance;
            int scaled_width = sprite_size * scale;
            int screen_x = get_screen_x(p->x, p->y, player);
            int cx = screen_width / 2;
            if (abs(screen_x - cx) < (scaled_width / 2) &&
                depth_buffer[cx] > distance)
            {
                if (distance < closest_distance)
                {
                    index_of_player_to_kill = player_index;
                }
            }
        }
    }
    if (index_of_player_to_kill != -1)
    {
        // TODO: Proper player death.
        randomly_spawn_player(players + index_of_player_to_kill);
    }
}
