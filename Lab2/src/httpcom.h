#ifndef HTTP_CON_H
#define HTTP_CON_H

#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>

class HttpCon {
public:
    static int m_epollfd;
    static int m_user_cnt;

    HttpCon() {
        m_socketfd = -1;
    }
    ~HttpCon() {}

    void process();
    //非阻塞的读写
    bool read();
    bool write();
    void init(int ,const struct sockaddr_in &);
    void close();

private:
    int m_socketfd;
    struct sockaddr_in m_addr;

};

#endif
