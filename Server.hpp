#include <vector>
#include <future>
#include <memory.h>
#include <unistd.h>
#include <threads.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/poll.h> 

using namespace std;

class Client
{
    private :
        
        string          ip_addr;
        int             time_connect;
        int             time_disconnect;
        
        bool            is_run;
        bool            is_loop;
        
        int             index;
        int             socket_fd;
        int             num_reconnect;
        
    public  :
        Client();
        ~Client();
        
        void            set_debug_print(void);

        void            set_index(int _index);
        
        bool            is_reconnect(string _ip_addr);
        bool            is_alive(void);    // get is_loop

        string          get_ip_addr(void);
        string          get_mac_addr(void);
        int             get_num_reconnect(void);
        int             get_timestamp(void);
        
        int             get_time_connect(void);
        int             get_time_disconnect(void);
        
        void            init(string _ip_addr, int _socket_fd);
        void            stop(void);
        void            run(void);
        void            reconnect(int _socket_fd);
        
        void            set_event_handler(void (*_func)(int, bool));
        void            set_queue_data(char *_data, int _size);

        void            execute(void);
};

class Server
{
    #define DFLT_NUM_MAX_CLIENT 20

    const int DFLT_SERV_PORT = 7000;

    const int SIZE_SCALE_SNDBUF = 10;
    const int SIZE_BACK_LOG = 40;

    const int TIME_ACCEPT_SEC = 1;
    const int TIME_ACCEPT_MSEC = 0;
    const int TIME_RCV_TIMEOUT = 2;

private:
    pollfd client_list[DFLT_NUM_MAX_CLIENT];
    std::mutex client_list_mutex;

    std::thread *accept_thread;

    bool is_accept_looping = false;

    int socket_fd = -1;

    void AcceptThreadFunc();
    void ServeClient();

public:
    Server(/* args */);
    ~Server();

    void Start();
    void Stop();
    void BroadCastToAllClient(string msg);
};