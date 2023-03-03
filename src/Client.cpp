#include "./Client.hpp"
#include "Client.hpp"

Client::Client(int id, std::string nickname)
{
    _id = id;
    _nickname = nickname;
}
int Client::get_client_id()
{
    return _id;
}

std::string Client::get_nickname()
{
    return _nickname;
}

Client::~Client()
{
}