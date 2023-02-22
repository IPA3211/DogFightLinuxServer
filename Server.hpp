#include <vector>
#include <future>
#include <memory.h>
#include <unistd.h>
#include <threads.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class Server
{
private:
    std::vector<int> *client_list;
    std::thread *accept_thread;
    void AcceptThreadFunc(int a);

public:
    Server(/* args */);
    ~Server();
};