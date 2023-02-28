#include "./Server.hpp"
#include "./json/json.h"
#include "./ThreadPool.hpp"
#include "./MySqlManager.hpp"

#define USE_SSL

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
}

void Server::Start()
{
#ifdef USE_SSL
    init_openssl();
#endif
    is_accept_looping = true;
    accept_thread = new std::thread([this]()
                                    { this->AcceptThreadFunc(); });
}

void Server::Stop()
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

void Server::BroadCastToAllClient(string msg)
{
    for (int i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        if (client_list[i].fd != -1)
        {
            send(client_list[i].fd, msg.c_str(), msg.length() + 1, 0);
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

    client_list[0].fd = socket_fd;
    client_list[0].events = POLLIN;

    for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        client_list[i].fd = -1;
    }

    maxi = 0;
#ifdef USE_SSL
    printf("ssl ");
#endif
    printf("server start!\n");
    while (is_accept_looping)
    {
        int nread = poll(client_list, maxi + i, -1);

        socklen_t clilen;
        struct sockaddr_in clientaddr;

        if (client_list[0].revents & POLLIN)
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
                if (client_list[i].fd < 0)
                {
                    client_list[i].fd = client_sockfd;
#ifdef USE_SSL
                    client_ssl_list[i] = ssl;
#endif
                    printf("accept!\n");
                    break;
                }
            }

            if (i == DFLT_NUM_MAX_CLIENT)
            {
                printf("max client!\n");
                close(client_sockfd);
#ifdef USE_SSL
                SSL_free(ssl);
#endif
                continue;
            }

            client_list[i].events = POLLIN;

            if (i > maxi)
            {
                maxi = i;
            }

            if (--nread <= 0)
                continue;
        }

        for (i = 1; i <= maxi; i++)
        {
            int sockfd = client_list[i].fd;
            if (sockfd < 0)
                continue;

            if (client_list[i].revents & (POLLIN | POLLERR))
            {
                int index = i;
                try
                {
                    auto root = RecvPacket(i);
                    auto result = pool.EnqueueJob([this, root, index]() -> void
                                                  { return this->ServeClient(root, index); });
                }
                catch (const FFError &e)
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

void Server::ServeClient(Json::Value packet, int index)
{
    std::cout << packet["index"].asInt() << endl;
    std::cout << packet["order"].asInt() << endl;
    std::cout << packet["msg"].asString() << endl;

    Json::Value ans_packet;
    Json::Value in_msg;

    Json::Reader reader;
    bool read = reader.parse(packet["msg"].asString(), in_msg);

    Json::StyledWriter writer;
    ans_packet["index"] = packet["index"].asInt();
    ans_packet["order"] = TcpPacketType::Answer;
    switch (packet["order"].asInt())
    {
    case TcpPacketType::DuplicationCheck:
        ans_packet["msg"] = writer.write(CheckDuplication(in_msg["table"].asString(), in_msg["column"].asString(), in_msg["check"].asString()));
        break;

    case TcpPacketType::SignUp:
        ans_packet["msg"] = writer.write(SignUpUser(in_msg["id"].asString(), in_msg["pw"].asString(), in_msg["nick"].asString(), in_msg["email"].asString()));
        break;
    }

    SendPacket(index, ans_packet);
}

void Server::SendPacket(int index, Json::Value packet)
{
    Json::StyledWriter writer;
    std::string outputConfig = writer.write(packet);
#ifdef USE_SSL
    SSL_write(client_ssl_list[index], outputConfig.c_str(), outputConfig.length() + 1);
#else
    send(client_list[index].fd, outputConfig.c_str(), outputConfig.length() + 1, 0);
#endif
}

Json::Value Server::RecvPacket(int index)
{
    string a;
    char buf[255];
    char line[255];

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
            close(client_list[index].fd);
#ifdef USE_SSL
            SSL_free(client_ssl_list[index]);
#endif
            client_list[index].fd = -1;
            break;
        }
        else
        {
            a += buf;
            if (recv_amt < sizeof(buf))
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
        std::cout << "Failed to parse Json" + a << endl;
        throw FFError((char *)"Failed to parse Json");
    }
    else
    {
        int fd = client_list[index].fd;
        return root;
    }
}

Json::Value Server::CheckDuplication(string table, string column, string check)
{
    Json::Value ans_value;
    MySqlManager manager;
    int temp_ans = 0;
    string msg = "";
    try
    {
        auto ans = manager.SendQuery("SELECT count(*) FROM dogfight." + table + " where " + column + " = \"" + check + "\";");
        auto row = mysql_fetch_row(ans);

        if (std::atoi(row[0]) == 0)
        {
            temp_ans = 1; // true
            msg = "true";
        }
        else
        {
            temp_ans = 0; // false
            msg = "false";
        }

        mysql_free_result(ans);
    }
    catch (const FFError &e)
    {
        temp_ans = -1; // error
        msg = e.Label;
    }

    ans_value["result"] = temp_ans;
    ans_value["msg"] = msg;

    return ans_value;
}

Json::Value Server::SignUpUser(string id, string pass, string nick, string email)
{
    Json::Value ans_value;
    MySqlManager manager;
    int temp_ans = 0;
    string msg = "";
    char query[255];
    try
    {
        sprintf(query, "Insert into dogfight.user(userId, passHash, nickName, email) values"
                       "('%s', '%s', '%s', '%s')",
                id.c_str(), pass.c_str(), nick.c_str(), email.c_str());
        auto ans = manager.SendQuery(query);

        temp_ans = 0;
        msg = "Sign in success";

        mysql_free_result(ans);
    }
    catch (const FFError &e)
    {
        temp_ans = -1; // error
        msg = e.Label;
    }

    ans_value["result"] = temp_ans;
    ans_value["msg"] = msg;

    return ans_value;
}