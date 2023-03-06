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
#ifdef USE_SSL
    init_openssl();
#endif
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

#ifdef USE_SSL
    cleanup_openssl();
#endif
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

#ifdef USE_SSL
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

#endif

    client_socket_list[0].fd = socket_fd;
    client_socket_list[0].events = POLLIN;
    room_data_list.push_back(new Room());

    for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        client_socket_list[i].fd = -1;
        client_socket_list[i].events = 0;
    }

    maxi = 0;
    i = 1;
#ifdef USE_SSL
    printf("ssl ");
#endif
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

#ifdef USE_SSL
            SSL *ssl = SSL_new(sslContext);
            SSL_set_fd(ssl, client_sockfd);
            int ssl_err = SSL_accept(ssl);
            if (ssl_err <= 0)
            {
                cout << "ssl_err" << endl;
                continue;
            }
#endif

            for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
            {
                if (client_socket_list[i].fd < 0)
                {
                    client_socket_list[i].fd = client_sockfd;
#ifdef USE_SSL
                    client_ssl_list[i] = ssl;
#endif
                    printf("%d : accept!\n", i);
                    break;
                }
            }

            if (i == DFLT_NUM_MAX_CLIENT)
            {
                printf("max client!\n");
                close(client_sockfd);
#ifdef USE_SSL
                SSL_free(ssl);
                ssl = 0;
#endif
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

#ifdef USE_SSL
    SSL_CTX_free(sslContext);
#endif
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
        throw(DFError((char *)"json parse error"));
    }

    Json::StyledWriter writer;
    ans_packet["index"] = packet["index"].asInt();
    ans_packet["order"] = TcpPacketType::Answer;

    switch (packet["order"].asInt())
    {
    case TcpPacketType::DuplicationCheck:
        ans_packet["msg"] = writer.write(mysqlManager->check_duplication(in_msg["column"].asInt(), in_msg["check"].asString()));
        break;

    case TcpPacketType::SignUp:
        ans_packet["msg"] = writer.write(mysqlManager->signup_user(in_msg["id"].asString(), in_msg["pw"].asString(), in_msg["nick"].asString(), in_msg["email"].asString()));
        break;

    case TcpPacketType::SignIn:
        ans_packet["msg"] = writer.write(mysqlManager->signin_user(in_msg["id"].asString(), in_msg["pw"].asString(), &client_data_list[index]));
        if (client_data_list[index] != nullptr)
        {
            client_data_list[index]->bind_socket(&client_socket_list[index], client_ssl_list[index]);
            client_data_list[index]->set_room(room_data_list[0]);
        }
        break;

    case TcpPacketType::Chat:
        ans_packet["msg"] = writer.write(send_chat(client_data_list[index], in_msg["msg"].asString()));
        break;
    }

#ifdef USE_SSL
    send_packet(client_ssl_list[index], ans_packet);
#else
    send_packet(&client_list[index], ans_packet);
#endif
}

#ifdef USE_SSL
void Server::send_packet(SSL *ssl, Json::Value packet)
{
    Json::StyledWriter writer;
    std::string outputConfig = writer.write(packet);
    SSL_write(ssl, outputConfig.c_str(), outputConfig.length() + 1);
}

#else
void Server::send_packet(pollfd *poll, Json::Value packet)
{
    Json::StyledWriter writer;
    std::string outputConfig = writer.write(packet);
    send(poll->fd, outputConfig.c_str(), outputConfig.length() + 1, 0);
}
#endif

Json::Value Server::recv_packet(int index)
{
    string a;
    char buf[1024];

    while (true)
    {
        memset(buf, 0x00, sizeof(buf));
#ifdef USE_SSL
        int recv_amt = SSL_read(client_ssl_list[index], (char *)buf, sizeof(buf));
#else
        int recv_amt = recv(client_list[index].fd, buf, sizeof(buf), 0);
#endif
        if (recv_amt <= 0)
        {
            close(client_socket_list[index].fd);
#ifdef USE_SSL
            SSL_free(client_ssl_list[index]);
            client_ssl_list[index] = nullptr;
#endif
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
        throw DFError((char *)"Failed to parse Json");
    }
    else
    {
        return root;
    }
}

Json::Value Server::send_chat(Client *client, string msg)
{
    Json::Value packet;
    Json::Value packet_msg;

    packet_msg["sender"] = client->get_nickname();
    packet_msg["msg"] = msg;

    Json::StyledWriter writer;

    packet["index"] = 0;
    packet["order"] = TcpPacketType::Chat;
    packet["msg"] = writer.write(packet_msg);

    auto client_list_temp = client->get_room()->get_client_list();

    Json::Value ans_packet;
    ans_packet["result"] = 1;
    ans_packet["msg"] = "success";

    try
    {
        for (auto &&c : client_list_temp)
        {
#ifdef USE_SSL
            send_packet(c->get_ssl(), packet);
#else
            send_packet(c->get_socket(), packet);
#endif
        }
    }
    catch (const std::exception &e)
    {
        ans_packet["result"] = -1;
        ans_packet["msg"] = e.what();
    }

    return ans_packet;
}
