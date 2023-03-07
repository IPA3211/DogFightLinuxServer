#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "./Client.hpp"
#include "./Server.hpp"
#include "./json/json.h"

struct RoomInfo
{
    Client *host;
    std::string name;
    bool is_private;
    std::string pw;
    int max_player;
};


class Room
{
private:
    RoomInfo info;
    std::vector<Client *> member_list;

public:
    Room(RoomInfo roominfo);
    ~Room();

    void join_client(Client *client);
    void exit_client(Client *client);

    void set_room_info(RoomInfo roominfo);
    
    RoomInfo get_room_info();
    int get_member_count();
    std::vector<Client *> get_client_list();
};

