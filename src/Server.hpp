#pragma once

#include <vector>
#include <future>
#include <memory.h>
#include <unistd.h>
#include <threads.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h> 
#include <iostream>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./json/json.h"
#include "./MySqlManager.hpp"
#include "./Client.hpp"
#include "./Room.hpp"

#define USE_SSL

using namespace std;

enum TcpPacketType{
    Answer = 0,
    Msg,

    DuplicationCheck = 100,

    SignUp = 200,
    SignIn,

    CreateRoom = 300,
    Quit = 999
};

class Server
{
    #define DFLT_NUM_MAX_CLIENT 20
    #define DFLT_NUM_MAX_ROOM 20

    const int DFLT_SERV_PORT = 7000;

    const int SIZE_SCALE_SNDBUF = 10;
    const int SIZE_BACK_LOG = 40;

    const int TIME_ACCEPT_SEC = 1;
    const int TIME_ACCEPT_MSEC = 0;
    const int TIME_RCV_TIMEOUT = 2;

private:
    pollfd client_socket_list[DFLT_NUM_MAX_CLIENT];
#ifdef USE_SSL
    SSL *client_ssl_list[DFLT_NUM_MAX_CLIENT];
#endif
    Client *client_data_list[DFLT_NUM_MAX_CLIENT] = {nullptr, };
    std::vector<Room *> room_data_list;

    std::mutex client_list_mutex;

    std::thread *accept_thread;
    MySqlManager *mysqlManager;

    bool is_accept_looping = false;

    int socket_fd = -1;

    void AcceptThreadFunc();

public:
    Server(/* args */);
    ~Server();

    void start();
    void stop();
    void broadCast_to_all_client(string msg);
    void serve_client(Json::Value packet, int fd);
    
#ifdef USE_SSL
    void send_packet(SSL *ssl, Json::Value packet);
#else
    void send_packet(pollfd *poll, Json::Value packet);
#endif
    Json::Value recv_packet(int index);
};