#pragma once

#include <iostream>
#include <vector>

#include "./Client.hpp"

class Room
{
private:
    std::string name;
    Client *host;
    std::vector<Client *> member;
public:
    Room();
    ~Room();

    void join_client(Client *client);
    void exit_client(Client *client);

    void send_packet_to_all();
};

