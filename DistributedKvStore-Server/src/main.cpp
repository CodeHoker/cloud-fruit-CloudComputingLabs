#include "all.hpp"
using namespace std;


int main(int argc, char* argv[] ) {
    //处理信号,防止因为客户端的突然终止而导致服务器崩溃
    set_signal(SIGPIPE, SIG_IGN);
    srandom((int)time(0));
    //epoll监听
        //epoll对象
    int fd_epoll = epoll_create(64);

    // 服务器初始化和运行
    RedisServer server;
    if(argc >= 2) 
        server.config_path.assign(argv[1]);
    server.init(fd_epoll,"andknow's RedisServer");
    server.serverRun();

    return 0;
}