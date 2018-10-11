/*
    Labyrinth
    Benedict Henshaw, 2018
    network.c - Server and client networking.
*/

void set_player_name(char * name)
{
    strncpy(player_name, name, sizeof(player_name));
}

void init_network()
{
    if (enet_initialize() != 0)
    {
        panic_exit("Could not initialise network systems.");
    }
    atexit(enet_deinitialize);
}

void create_network(int port)
{
    if (!port) port = DEFAULT_PORT;
    ENetAddress address = { .host = ENET_HOST_ANY, .port = port };
    if (local_host) enet_host_destroy(local_host);
    local_host = enet_host_create(&address, max_players, 2, 0, 0);
    if (local_host == NULL) panic_exit("Could not create server at port '%d'.", port);
    push_console_string("Server launched.");

    network_mode = NETMODE_SERVER;
}

void join_network(char * address_with_optional_port)
{
    if (local_host) enet_host_destroy(local_host);
    local_host = enet_host_create(NULL, 1, 2, 0, 0);
    if (local_host == NULL) panic_exit("Could not create network client.");

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = DEFAULT_PORT;

    remote_server = enet_host_connect(local_host, &address, 2, 0);
    if (remote_server == NULL)
    {
        push_console_string("Could not connect to '%s'", address_with_optional_port);
        return;
    }

    push_console_string("Connecting...");
    // TODO: Is it possible to perform connections asynchronously to avoid the lag?
    clear_entry();
    draw_console();
    display_screen();

    ENetEvent event;
    if (enet_host_service(local_host, &event, 3000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        push_console_string("Connected to '%s'", address_with_optional_port);
    }
    else
    {
        enet_peer_reset(remote_server);
        remote_server = NULL;
        push_console_string("Connection failed.");
    }

    network_mode = NETMODE_CLIENT;
}

void handle_network()
{
    if (local_host)
    {
        ENetEvent event;
        while (enet_host_service(local_host, &event, 0))
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
            {
                push_console_string((char *)event.packet->data);
            }
            else if (event.type == ENET_EVENT_TYPE_CONNECT)
            {
                push_console_string("Connection from %x:%u.",
                    event.peer->address.host,
                    event.peer->address.port);
            }
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
            {
                push_console_string("Disconnection by %x:%u.",
                    event.peer->address.host,
                    event.peer->address.port);
            }
        }
    }
}

void send_string_over_network(char * string)
{
    if (network_mode)
    {
        ENetPacket * packet = enet_packet_create(string, strlen(string) + 1, ENET_PACKET_FLAG_RELIABLE);
        if (network_mode == NETMODE_CLIENT)      enet_peer_send (remote_server, 0, packet);
        else if (network_mode == NETMODE_SERVER) enet_host_broadcast(local_host, 0, packet);
    }
}
