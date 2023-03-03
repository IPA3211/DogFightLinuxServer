#include "./Client.hpp"
#include "./Room.hpp"

Client::Client(int id, std::string nickname)
{
    _id = id;
    _nickname = nickname;
}
Client::~Client()
{
    if(room != nullptr){
        room->exit_client(this);
        room = nullptr;
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

void Client::bind_socket(pollfd *poll, SSL *ssl_sock)
{
    socket = poll;
    ssl = ssl_sock;
}