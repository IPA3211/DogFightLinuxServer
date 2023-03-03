#pragma once

#include <iostream>

class Client
{
private:
    int _id;
    std::string _nickname;

public:
    Client(int id, std::string nickname);
    ~Client();

    int get_client_id();
    std::string get_nickname();
};