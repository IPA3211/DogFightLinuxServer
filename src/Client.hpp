#pragma once

#include <sys/poll.h> 
#include <iostream>

#include <openssl/ssl.h>
#include <openssl/err.h>

class Room;

class Client
{
private:
    int _id;
    std::string _nickname;

    pollfd *_socket;
    SSL *_ssl;

    Room *_room = nullptr;

public:
    Client(int id, std::string nickname);
    ~Client();

    int get_client_id();
    std::string get_nickname();
    void bind_socket(pollfd *poll, SSL *ssl_sock);
    void set_room(Room *room);
};