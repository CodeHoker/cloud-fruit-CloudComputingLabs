#ifndef NET_HPP_
#define NET_HPP_
#include "all.hpp"

// --------------------------------网络编程部分
    // 网络编程的汇总
// int my_serverSocket();
    // 利用sigaction设置信号和函数处理
void set_signal(int sig, void (*handler)(int));
    // EPOLL
     //可监听事务的个数 
// const int MAX_EVENT = 32;
    // 设置EPOLLIN/EPOLLOUT,是否需要携带ONESHOT
enum EpollOpt{EP_OUT = 0, EP_IN};
void add_event(int fd_epoll, int fd, bool one_short, EpollOpt opt);
    // 清除epoll中某个文件描述符,并且关闭
void delete_event(int fd_epoll, int fd_socket);
    // 默认是ONESHOT和REHUP
void mod_event(int fd_epoll, int fd_socket,  int ev);



#endif