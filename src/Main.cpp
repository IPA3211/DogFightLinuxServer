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
    server->Start();

    string input;
    
    while(true){
        cin >> input;
        
        if(input == "q"){
            auto aa = std::async([server] () {server->Stop ();});
            break;
        }
        else{
            server -> BroadCastToAllClient(input);
        }
    }
}