#include "Userall.hpp"
using namespace std;

// --------------------------------网络编程部分
    // 网络编程的汇总

enum EpollOpt{EP_OUT = 0, EP_IN};
void set_signal(int sig, void (*handler)(int));
void add_event(int fd_epoll, int fd, bool one_short, EpollOpt opt);
void delete_event(int fd_epoll, int fd_socket);
void mod_event(int fd_epoll, int fd_socket,  int ev);

string encodeResp(const vector<string>& resp, size_t pos);
vector<string> decodeResp(const char* buffer, string& str);