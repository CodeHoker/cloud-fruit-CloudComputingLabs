#include "httpcon.h"

//利用sigaction设置信号和函数处理
void set_signal(int sig, void (*handler)(int)) {
    struct sigaction siga;
    bzero(&siga, sizeof(siga));
    siga.sa_handler = handler;
    //设定阻塞
    sigfillset(&siga.sa_mask);
    sigaction(sig,&siga,NULL);
}

void add_event(int fd_epoll, int fd, bool one_short) {
    epoll_event event;
    event.data.fd = fd;
    //监听数据读取和连接断开
    event.events = EPOLLIN | EPOLLHUP ;
    //oneshort:防止一个文件描述符被多个事件触发
    if(one_short) {
        event.events |= EPOLLONESHOT;
    }   

    epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞,和ET,LT之间的关系？
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, flag, NULL);
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

int HttpCon::m_epollfd = -1;
int HttpCon::m_user_cnt = 0;

void HttpCon::init(int sockfd,const sockaddr_in &addr) {
    m_socketfd = sockfd;
    m_addr = addr;

    int flag_set = 1;
    setsockopt(m_socketfd, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));
    add_event(m_epollfd, sockfd, true);

    m_user_cnt ++;
}

void HttpCon::close() {
    if(m_socketfd != -1) {
        delete_event(m_epollfd, m_socketfd);
        m_socketfd = -1;
        m_user_cnt --;
    }

}

bool HttpCon::read() {
    printf("非阻塞一次性读出所有数据\n");
    return true;
}

bool HttpCon::write() {
    printf("非阻塞写入数据\n");
    return true;
}

void HttpCon::process() {
    //工作主要内容
    printf("工作执行中......\n");
}

