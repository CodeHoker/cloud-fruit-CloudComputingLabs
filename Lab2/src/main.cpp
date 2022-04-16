#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "threadpool.h"
#include "httpcon.h"

#define MAX_REQUESTS 1024
#define MAX_EVENT 1024


extern void set_signal(int sig, void (*handler)(int));
extern void add_event(int fd_epoll, int fd, bool one_short);
extern void delete_event(int fd_epoll, int fd_socket);
extern void mod_event(int fd_epoll, int fd_socket, int ev);

int main(int argc, char *argv[]) {
    //参数
    if(argc <= 1) {
        printf("need : <port>\n");
        exit(-1);
    }

    //初始化
    //处理信号
    set_signal(SIGPIPE, SIG_IGN);

    //初始化线程池
    ThreadPool<HttpCon> *pool = NULL;
    try {
        pool = new ThreadPool<HttpCon>;
        printf("线程池创建成功\n");
    } catch(...) {
        exit(-1);
    }


    HttpCon *Requests = new HttpCon[MAX_REQUESTS];

    //网络编程代码
        //socket()
    int fd_server = socket(PF_INET, SOCK_STREAM, 0);

        // 设置端口复用
    int flag_set = 1;
    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));

        //bind()
    //需要<arpa/inet.h>
    struct sockaddr_in addr_server;
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(atoi(argv[1]));
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd_server, (struct sockaddr*)&addr_server, sizeof(addr_server));

        //listen()
    listen(fd_server, 5);

    //epoll监听
        //epoll对象
    epoll_event events[MAX_EVENT];
    int fd_epoll = epoll_create(64);

        //添加监听文件
    add_event(fd_epoll, fd_server, false);
    HttpCon::m_epollfd = fd_epoll;

        //开始监听
    while(true) {
        sleep(1);
        printf("等待监听事件......\n");
        int num = epoll_wait(fd_epoll, events, MAX_EVENT, -1);
        if((num < 0) && (errno != EINTR)) {
            printf("epoll_wait()\n");
            break;
        }
        printf("事件到来......\n");
        
        for(int i = 0; i < num ; i++) {
            int fd_event = events[i].data.fd;
            if(fd_event == fd_server) {
                //服务器监听套接字响应，有新的客户端连接
                printf("监听到客户端连接...\n");
                struct sockaddr_in addr_client;
                socklen_t len_addr = sizeof(addr_client); 
                int fd_client = accept(fd_server, (struct sockaddr*)&addr_client, &len_addr);

                if(HttpCon::m_user_cnt >= MAX_REQUESTS) {
                    close(fd_client);
                    continue;
                }
                Requests[fd_client].init(fd_client, addr_client);
                
            } else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //对面发生异常断开或者错误监听
                Requests[fd_event].close();

            } else if(events[i].events & EPOLLIN) {
                if(Requests[fd_event].read()) {
                    pool->append(Requests + fd_event);
                } else {
                    Requests[fd_event].close();
                }

            } else if(events[i].events & EPOLLOUT) {
                //这里的信号是什么时候才会被接收到呢？？？
                if(!Requests[fd_event].write()) {
                    Requests[fd_event].close();
                }

            }
        } 
    }

    close(fd_epoll);
    close(fd_server);
    delete[] Requests;
    delete pool;

    return 0;
}