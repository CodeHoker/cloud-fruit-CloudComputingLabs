#include "Userall.hpp"
using namespace std;

int main(int argc, char* argv[]) {

    set_signal(SIGPIPE, SIG_IGN);
    int fd_epoll = epoll_create(64);

    UserServer server;
    if(argc >= 2) 
        server.config_path.assign(argv[1]);
    server.init(fd_epoll);
    server.serverRun();
    return 0;
} 