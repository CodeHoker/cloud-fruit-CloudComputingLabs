#include "Userall.hpp"
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

string encodeResp(const vector<string>& resp, size_t pos) {
    string ret;
    ret.assign(resp[pos]);
    switch (resp[pos][0])
    {
    case '+': ;
    case '-': ;
    case ':': ;
        ret.append(resp[pos+1]);
        ret.append("\r\n");
        break;
    case '$':
        ret.append(to_string(resp[pos+1].size()));
        ret.append("\r\n");
        ret.append(resp[pos+1]);
        ret.append("\r\n");
        break;
    case '*':
        ret.append(to_string(resp.size()/2));
        ret.append("\r\n");
        pos++;
        while(pos <= resp.size()-2) {
            ret.append(encodeResp(resp, pos));
            pos += 2;
        };
        break;
    }
    return ret;
}

vector<string> decodeResp(const char* buffer, string& str) {
    // cout << "即将解析的数据\n" << buffer;
    str.assign(buffer);
    string str_t;
    vector<string> ret;
    // *2\r\n$5\r\nCloud\r\n$9\r\nComputing\r\n
    switch(buffer[0]) {
        case '-':
        case '+':
        case ':':
            ret.emplace_back(str.substr(1, str.length()-3));
        case '*':
            bool flag_isStr = false;//记录是否读取一个字符串
            bool flag_isdollar = false;//记录是否读取到一个'$'
            for(int i = 1;i < str.length(); i++) {
                if(str[i] == '\r' && flag_isStr) {
                    flag_isdollar = false;
                    flag_isStr = false;
                    ret.emplace_back(str_t);
                } else if(str[i] == '\n' && flag_isdollar) {
                    flag_isStr = true;
                    str_t.assign("");
                } else if(str[i] == '$') {
                    flag_isdollar = true;
                } else if(flag_isStr)
                    str_t.push_back(str[i]);
            }

        break;
    }
    return ret;
}