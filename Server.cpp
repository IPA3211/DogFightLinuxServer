#include "./Server.hpp"

Server::Server(/* args */)
{
    client_list = new std::vector<int>();
    accept_thread = new std::thread([this](){ this->AcceptThreadFunc(10);});

    client_list->push_back(10);
}

void Server::AcceptThreadFunc(int num_port)
{
    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(num_port);

    if (bind(serv_sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("bind() error");
        exit(1);
    }

    while (1)
    {
        if (listen(serv_sock, 5) == -1)
        {
            printf("listen() error");
            exit(1);
        }

        sockaddr clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    }
}

Server::~Server()
{
}
