#include "./Server.hpp"
#include "./ThreadPool.hpp"
#include "./DFError.hpp"
#include "Server.hpp"

void init_openssl()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    cout << "Start SSL" << endl;
}

void cleanup_openssl()
{
    ERR_free_strings();
    EVP_cleanup();
    cout << "CleanUp SSL" << endl;
}

Server::Server(/* args */)
{
}

Server::~Server()
{
    if (accept_thread)
    {
        if (accept_thread->joinable())
        {
            accept_thread->join();
        }
    }

    for (int i = 0; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        delete client_data_list[i];
    }

    for (auto &&i : room_data_list)
    {
        delete i;
    }

    delete accept_thread;
    delete mysqlManager;
}

void Server::start()
{
    init_openssl();
    is_accept_looping = true;
    mysqlManager = new MySqlManager();
    accept_thread = new std::thread([this]()
                                    { this->AcceptThreadFunc(); });
}

void Server::stop()
{
    is_accept_looping = false;

    if (socket_fd != -1)
    {
        shutdown(socket_fd, SHUT_RDWR);
        socket_fd = -1;
        printf("Stop() : Server Stopped\n");
    }

    if (accept_thread)
    {
        if (accept_thread->joinable())
        {
            accept_thread->join();
        }
    }

    cleanup_openssl();
}

void Server::broadCast_to_all_client(string msg)
{
    for (int i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        if (client_socket_list[i].fd != -1)
        {
            send(client_socket_list[i].fd, msg.c_str(), msg.length() + 1, 0);
        }
    }
}

void Server::AcceptThreadFunc()
{
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    ThreadPool::ThreadPool pool(8);

    sockaddr_in serv_addr;
    int i, maxi;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(DFLT_SERV_PORT);

    if (bind(socket_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("bind() error");
        exit(0);
    }

    if (listen(socket_fd, 5) == -1)
    {
        printf("listen() error");
        exit(0);
    }

    SSL_CTX *sslContext = SSL_CTX_new(SSLv23_server_method());
    SSL_CTX_set_options(sslContext, SSL_OP_SINGLE_DH_USE);

    if (SSL_CTX_use_certificate_file(sslContext, "../test.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
    }
    if (SSL_CTX_use_PrivateKey_file(sslContext, "../key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
    }

    client_socket_list[0].fd = socket_fd;
    client_socket_list[0].events = POLLIN;

    client_data_list[0] = new Client(0, "Server");

    RoomInfo roominfo;
    roominfo.host = client_data_list[0];
    roominfo.max_player = DFLT_NUM_MAX_CLIENT;
    room_data_list.push_back(new Room(roominfo, this));

    for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        client_socket_list[i].fd = -1;
        client_socket_list[i].events = 0;
    }

    maxi = 0;
    i = 1;
    printf("ssl ");
    printf("server start!\n");
    while (is_accept_looping)
    {
        int nread = poll(client_socket_list, i + maxi, -1); // valgrind error -1인 곳 접근하면 그런듯?

        socklen_t clilen;
        struct sockaddr_in clientaddr;

        if (client_socket_list[0].revents & POLLIN)
        {
            clilen = sizeof(clientaddr);
            int client_sockfd = accept(socket_fd,
                                       (struct sockaddr *)&clientaddr,
                                       &clilen);

            SSL *ssl = SSL_new(sslContext);
            SSL_set_fd(ssl, client_sockfd);
            int ssl_err = SSL_accept(ssl);
            if (ssl_err <= 0)
            {
                cout << "ssl_err" << endl;
                continue;
            }

            for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
            {
                if (client_socket_list[i].fd < 0)
                {
                    client_socket_list[i].fd = client_sockfd;
                    client_ssl_list[i] = ssl;
                    printf("%d : accept!\n", i);
                    break;
                }
            }

            if (i == DFLT_NUM_MAX_CLIENT)
            {
                printf("max client!\n");
                close(client_sockfd);
                SSL_free(ssl);
                ssl = 0;
                continue;
            }

            client_socket_list[i].events = POLLIN;

            if (i > maxi)
            {
                maxi = i;
            }

            if (--nread <= 0)
                continue;
        }

        for (i = 1; i <= maxi; i++)
        {
            int sockfd = client_socket_list[i].fd;
            if (sockfd < 0)
                continue;

            if (client_socket_list[i].revents & (POLLIN | POLLERR))
            {
                int index = i;
                try
                {
                    auto root = recv_packet(i);
                    auto result = pool.EnqueueJob([this, root, index]() -> void
                                                  { return this->serve_client(root, index); });
                }
                catch (const DFError &e)
                {
                    cout << e.Label << endl;
                }
            }
        }
    }

    SSL_CTX_free(sslContext);
}

void Server::serve_client(Json::Value packet, int index)
{
    std::cout << packet["index"].asInt() << endl;
    std::cout << packet["order"].asInt() << endl;
    std::cout << packet["msg"].asString() << endl;

    Json::Value ans_packet;
    Json::Value in_msg;

    Json::Reader reader;
    bool read = reader.parse(packet["msg"].asString(), in_msg);

    if (!read)
    {
        throw(DFError(ERR_JSON_PARSE));
    }

    Json::Value msg;

    switch (packet["order"].asInt())
    {
    case TcpPacketType::DuplicationCheck:
        msg = duplication_check(index, in_msg);
        break;
    case TcpPacketType::SignUp:
        msg = sign_up(index, in_msg);
        break;
    case TcpPacketType::SignIn:
        msg = sign_in(index, in_msg);
        break;
    case TcpPacketType::GetUserInfo:
        msg = get_user_info(index, in_msg);
        break;
    case TcpPacketType::Chat:
        msg = send_chat(index, in_msg);
        break;
    case TcpPacketType::RoomCreate:
        msg = room_create(index, in_msg);
        break;
    case TcpPacketType::RoomJoin:
        msg = room_join(index, in_msg);
        break;
    case TcpPacketType::GetRoomList:
        msg = room_list(index, in_msg);
        break;
    }
    send_packet(client_ssl_list[index], packet["index"].asInt(), TcpPacketType::Answer, msg);
}

Json::Value Server::duplication_check(int index, Json::Value msg)
{
    return mysqlManager->check_duplication(msg["column"].asInt(), msg["check"].asString());
}

Json::Value Server::sign_up(int index, Json::Value msg)
{
    return mysqlManager->signup_user(msg["id"].asString(), msg["pw"].asString(), msg["nick"].asString(), msg["email"].asString());
}

Json::Value Server::sign_in(int index, Json::Value msg)
{
    Json::Value ans = mysqlManager->signin_user(msg["id"].asString(), msg["pw"].asString(), &client_data_list[index]);
    if (client_data_list[index] != nullptr)
    {
        client_data_list[index]->bind_socket(&client_socket_list[index], client_ssl_list[index]);
        try
        {
            client_data_list[index]->set_room(room_data_list[0], "");
        }
        catch (const DFError &e)
        {
            ans["result"] = -1;
            ans["msg"] = e.Label;
        }
    }
    return ans;
}

Json::Value Server::get_user_info(int index, Json::Value msg)
{
    return mysqlManager->get_user_info(client_data_list[index]);
}

Json::Value Server::send_chat(int index, Json::Value msg)
{
    Json::StyledWriter writer;
    Json::Value ans_packet;
    Json::Value packet_msg;

    packet_msg["sender"] = client_data_list[index]->get_nickname();
    packet_msg["msg"] = msg["msg"].asString();

    ans_packet = client_data_list[index]->get_room()->send_packet_all(0, TcpPacketType::Chat, packet_msg);
    return ans_packet;
}

Json::Value Server::room_create(int index, Json::Value msg)
{
    Json::Value ans;
    RoomInfo info;
    info.host = client_data_list[index];
    info.is_private = msg["private"].asBool();
    info.max_player = msg["max"].asInt();
    info.name = msg["name"].asString();
    info.pw = msg["pw"].asString();

    auto room = new_room(info);
    try
    {
        client_data_list[index]->set_room(room, msg["pw"].asString());
        ans["result"] = 1;
        ans["msg"] = "Room Create Success";
    }
    catch (const DFError &e)
    {
        delete_room(room);
        ans["result"] = -1;
        ans["msg"] = e.Label;
    }
    return ans;
}

Json::Value Server::room_join(int index, Json::Value msg)
{
    Json::Value ans;
    try
    {
        auto name = msg["name"].asString();
        auto hostName = msg["host"].asString();
        auto room = find_if(room_data_list.begin(), room_data_list.end(), [name, hostName](Room *r)
                            { return (r->get_room_info().name == name) && (r->get_room_info().host->get_nickname() == hostName); });

        if (room == room_data_list.end())
        {
            ans["result"] = -1;
            ans["msg"] = "no room";
        }
        else
        {
            client_data_list[index]->set_room(*room, msg["pw"].asString());
        }
    }
    catch (const DFError &e)
    {
        ans["result"] = -1;
        ans["msg"] = e.Label;
    }
    return ans;
}

Json::Value Server::room_list(int index, Json::Value msg)
{
    Json::Value ans;
    ans["result"] = 1;
    ans["msg"] = "";
    {
        for (auto &&room_data : room_data_list)
        {
            auto room_info = room_data->get_room_info();
            if (room_info.name != "")
            {
                Json::Value pack;
                pack["name"] = room_info.name;
                pack["isPrivate"] = room_info.is_private;
                pack["maxPlayer"] = room_info.max_player;
                pack["nickname"] = room_info.host->get_nickname();
                pack["curPlayer"] = room_data->get_member_count();

                send_packet(client_ssl_list[index], 0, TcpPacketType::GetRoomList, pack);
            }
        }
    }

    return ans;
}

Room *Server::new_room(RoomInfo info)
{
    Room *room = new Room(info, this);
    room_data_list.push_back(room);
    return room;
}

void Server::delete_room(Room *room)
{
    room_data_list.erase(remove(room_data_list.begin(), room_data_list.end(), room), room_data_list.end());
    delete room;
}

void Server::send_packet(SSL *ssl, Json::Value packet)
{
    Json::StyledWriter writer;
    std::string outputConfig = writer.write(packet);
    SSL_write(ssl, outputConfig.c_str(), outputConfig.length() + 1);
}

void Server::send_packet(SSL *ssl, int index, TcpPacketType type, Json::Value msg)
{
    Json::Value packet;
    Json::StyledWriter writer;
    packet["index"] = index;
    packet["order"] = type;
    packet["msg"] = writer.write(msg);

    send_packet(ssl, packet);
}

Json::Value Server::recv_packet(int index)
{
    string a;
    char buf[1024];

    while (true)
    {
        memset(buf, 0x00, sizeof(buf));
        int recv_amt = SSL_read(client_ssl_list[index], (char *)buf, sizeof(buf));

        if (recv_amt <= 0)
        {
            close(client_socket_list[index].fd);
            SSL_free(client_ssl_list[index]);
            client_ssl_list[index] = nullptr;
            client_socket_list[index].fd = -1;

            if (client_data_list[index] != nullptr)
            {
                delete client_data_list[index];
                client_data_list[index] = nullptr;
            }

            printf("%d : bye!\n", index);
            break;
        }
        else
        {
            a += buf;
            if (recv_amt < (int)sizeof(buf))
            {
                break;
            }
        }
    }

    Json::Reader reader;
    Json::Value root;
    bool parsingRet = reader.parse(a, root);

    if (parsingRet == false)
    {
        throw DFError(ERR_JSON_PARSE);
    }
    else
    {
        return root;
    }
}