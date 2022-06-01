#include "all.hpp"
using namespace std;

void set_signal(int sig, void (*handler)(int)) {
    struct sigaction siga;
    bzero(&siga, sizeof(siga));
    siga.sa_handler = handler;
    //设定阻塞
    sigfillset(&siga.sa_mask);
    sigaction(sig,&siga,NULL);
}


void add_event(int fd_epoll, int fd, bool one_short, EpollOpt opt) {
    epoll_event event;
    event.data.fd = fd;
    //监听数据读取和连接断开
    event.events = EPOLLIN | EPOLLRDHUP ;
    if(opt == EpollOpt::EP_IN) {
        event.events = EPOLLIN  | EPOLLRDHUP ;
    }else if(opt == EpollOpt::EP_OUT) {
        event.events = EPOLLOUT | EPOLLRDHUP ;
    }
    //oneshort:防止一个文件描述符被多个事件触发
    if(one_short) {
        event.events |= EPOLLONESHOT;
    }   

    epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞,和ET,LT之间的关系？
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd,F_SETFL, flag, NULL);
}

void delete_event(int fd_epoll, int fd_socket) {
    epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_socket, NULL);
    close(fd_socket);
}

void mod_event(int fd_epoll, int fd_socket, int ev) {
    epoll_event event;
    event.data.fd = fd_socket;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(fd_epoll, EPOLL_CTL_MOD ,fd_socket, &event);
}

// int my_serverSocket() {
//         //socket()
//     int fd_server = socket(PF_INET, SOCK_STREAM, 0);
//     if(fd_server == -1)
//         handler_error("socket()");

//     // 设置端口复用
//     int flag_set = 1;
//     setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));

//     //bind()需要<arpa/inet.h>
//     struct sockaddr_in addr_server;
//     addr_server.sin_family = AF_INET;
//     addr_server.sin_port = htons(stoi(config["serverPort"]));
//     addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
//     bind(fd_server, (struct sockaddr*)&addr_server, sizeof(addr_server));

//         //listen()
//     listen(fd_server, 15);

//     return fd_server;
// }