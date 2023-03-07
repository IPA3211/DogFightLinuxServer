#pragma once

#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>

#include "./Enums.hpp"
#include "./Client.hpp"
#include "./json/json.h"

class Server;

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
    Server *server;
    RoomInfo info;
    
    std::mutex room_mutex;
    std::vector<Client *> member_list;

public:
    Room(RoomInfo roominfo, Server *server);
    ~Room();

    void join_client(Client *client, std::string pw);
    void exit_client(Client *client);

    void set_room_info(RoomInfo roominfo);
    
    RoomInfo get_room_info();
    int get_member_count();

    Json::Value send_packet_all(int index, TcpPacketType type, Json::Value msg);
};

