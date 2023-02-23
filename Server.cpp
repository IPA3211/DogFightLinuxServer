#include "./Server.hpp"
#include "./json/json.h"

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

    sockaddr_in serv_addr;
    int i, maxi;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(DFLT_SERV_PORT);

    if (bind(socket_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("bind() error");
    }

    if (listen(socket_fd, 5) == -1)
    {
        printf("listen() error");
    }

    client_list[0].fd = socket_fd;
    client_list[0].events = POLLIN;

    for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
    {
        client_list[i].fd = -1;
    }

    maxi = 0;
    char buf[255];
    char line[255];

    printf("server start!");
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

            for (i = 1; i < DFLT_NUM_MAX_CLIENT; i++)
            {
                if (client_list[i].fd < 0)
                {
                    client_list[i].fd = client_sockfd;
                    printf("accept!\n");
                    break;
                }
            }

            if (i == DFLT_NUM_MAX_CLIENT)
            {
                printf("max client!\n");
                close(client_sockfd);
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

            string a;

            if (client_list[i].revents & (POLLIN | POLLERR))
            {
                while (true)
                {
                    memset(buf, 0x00, sizeof(buf));
                    int recv_amt = recv(sockfd, buf, sizeof(buf), 0);
                    if (recv_amt <= 0)
                    {
                        close(client_list[i].fd);
                        client_list[i].fd = -1;
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
                }
            }
        }
    }
}

void Server::ServeClient()
{
    int rc;
    struct timeval t_timeout;

    // device의 open이 되었다고 가정하고 진행한다.
    int dev_fd = 3;
    int max_fd = dev_fd + 1;

    fd_set fd_reads;
}

void Server::Start()
{
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
}
