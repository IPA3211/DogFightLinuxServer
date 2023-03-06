#include "./Client.hpp"
#include "./Room.hpp"
#include "Client.hpp"

Client::Client(int id, std::string nickname)
{
    _id = id;
    _nickname = nickname;
}
Client::~Client()
{
    if (_room != nullptr)
    {
        _room->exit_client(this);
        _room = nullptr;
    }
}

int Client::get_client_id()
{
    return _id;
}

std::string Client::get_nickname()
{
    return _nickname;
}

SSL *Client::get_ssl()
{
    return _ssl;
}

pollfd *Client::get_socket()
{
    return _socket;
}

void Client::bind_socket(pollfd *poll, SSL *ssl_sock)
{
    _socket = poll;
    _ssl = ssl_sock;
}

void Client::set_room(Room *room)
{
    if (room != nullptr)
    {
        _room = room;
        _room->join_client(this);
    }
}

Room *Client::get_room()
{
    return _room;
}
