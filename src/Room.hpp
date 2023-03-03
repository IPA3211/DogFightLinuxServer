#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "./Client.hpp"
#include "./Server.hpp"
#include "./json/json.h"

class Room
{
private:
    std::string name;
    Client *host;
    std::vector<Client *> member_list;
public:
    Room();
    ~Room();

    void join_client(Client *client);
    void exit_client(Client *client);

    std::vector<Client *> get_client_list();
};

