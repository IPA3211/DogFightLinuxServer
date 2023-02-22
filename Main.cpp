#include <iostream>
#include <memory.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }

    int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("bind() error");
        exit(1);
    }

    if (listen(serv_sock, 5) == -1)
    {
        printf("listen() error");
        exit(1);
    }

    sockaddr clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

    if (clnt_sock == -1)
    {
        printf("accept() error");
        exit(1);
    }

    char message[30];
    int str_len;

    str_len = read(clnt_sock, message, sizeof(message) - 1);
    if (str_len == -1)
    {
        printf("read() error");
        exit(1);
    }

    printf("Client : %s", message);

    char send_message[] = "Hello client";
    write(clnt_sock, send_message, sizeof(send_message));
    close(clnt_sock);
    close(serv_sock);
}