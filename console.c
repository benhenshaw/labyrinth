/*
    Labyrinth
    Benedict Henshaw, 2018
    console.c - Management of the in-game console.
*/

void clear_console()
{
    for (int line_index = 0; line_index < console_line_count; ++line_index)
    {
        for (int char_index = 0; char_index < console_width; ++char_index)
        {
            console_buffer[line_index][char_index] = '\0';
        }
    }
}

void push_console_string(char * string, ...)
{
    for (int i = console_line_count - 1; i >= 1; --i)
    {
        strncpy(console_buffer[i], console_buffer[i-1], console_width);
    }

    va_list args;
    va_start(args, string);
    char buffer[console_width];
    vsnprintf(buffer, console_width, string, args);
    va_end(args);

    strncpy(console_buffer[0], buffer, console_width);
}

void draw_console()
{
    for (int line_index = 0; line_index < console_line_count; ++line_index)
    {
        draw_text(5, screen_height - 5
                - (font_char_height * 2)
                - (line_index * font_char_height),
            ~0, console_buffer[line_index]);
    }

    if (entry_active)
    {
        draw_text(5, screen_height - 5 - font_char_height, ~0, "> %s", entry);
        if ((SDL_GetTicks() / 512) & 1)
        {
            draw_text(5 + (entry_index + 2) * font_char_width,
                screen_height - (5) - font_char_height,
                ~0, "_");
        }
    }
}

void push_entry_character(char c)
{
    if (c >= ' ' && c <= '~' && entry_index < console_width)
    {
        entry[entry_index] = c;
        entry[entry_index+1] = '\0';
        ++entry_index;
    }
}

void remove_entry_character()
{
    if (entry_index > 0)
    {
        --entry_index;
        entry[entry_index] = '\0';
    }
}

void clear_entry()
{
    memset(entry, 0, sizeof(entry));
    entry_index = 0;
}

bool parse_command(char * string)
{
    if (string && string[0] == '/' && string[1] != '\0')
    {
        int length = strlen(string);
        char * cmd = string + 1;
        int cmd_length = length - 1;
        char * arg = string;
        for (int i = 1; i < length; ++i)
        {
            if (isspace(string[i])) {
                arg += i + 1;
                cmd_length = i - 1;
                break;
            }
        }

        if (arg == string) arg = NULL;

        // TODO: (command_name, function, arguments) list, with type checking.
        #define CMD(s) strncmp(string+1, #s, cmd_length) == 0

        if (CMD(quit))
        {
            exit(0);
        }
        else if (CMD(echo))
        {
            if (arg) push_console_string(arg);
        }
        else if (CMD(host))
        {
            create_network(0);
        }
        else if (CMD(join))
        {
            if (arg) join_network(arg);
        }
        else if (CMD(name))
        {
            if (arg)
            {
                set_player_name(arg);
                push_console_string("Set name to '%s'.", arg);
            }
            else
            {
                push_console_string("Usage: /name your_name_here");
            }
        }
        else if (CMD(fov))
        {
            if (arg) view_angle = strtod(arg, NULL);
        }
        else if (CMD(view_accuracy))
        {
            if (arg) view_accuracy = strtod(arg, NULL);
        }
        else if (CMD(fullscreen))
        {
            toggle_fullscreen();
        }
        else if (CMD(clear))
        {
            clear_console();
        }
        else if (CMD(help))
        {
            push_console_string("Commands: ");
            push_console_string("  quit echo help host clear");
            push_console_string("  join name fullscreen");
        }
        else
        {
            push_console_string("Unknown command '%.*s'.", cmd_length, cmd);
            return false;
        }

        #undef CMD
        return true;
    }
    return false;
}

void submit_entry()
{
    if (entry[0] && entry_index)
    {
        if (!parse_command(entry) && entry[0] != '/')
        {
            push_console_string(entry);
            if (local_host)
            {
                send_string_over_network(entry);
            }
        }
        strncpy(previous_entry, entry, console_width);
        memset(entry, 0, console_width);
        entry_index = 0;
    }
    entry_active = false;
}

void load_previous_entry()
{
    strncpy(entry, previous_entry, console_width);
    entry_index = strlen(entry);
}
