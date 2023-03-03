#include <iostream>
#include <memory.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <future>

#include "Server.hpp"

int main(int argc, char *argv[])
{
    Server *server = new Server();
    server->start();

    string input;
    
    while(true){
        cin >> input;
        
        if(input == "q"){
            auto aa = std::async([server] () {server->stop ();});
            break;
        }
        else{
            server -> broadCast_to_all_client(input);
        }
    }
}