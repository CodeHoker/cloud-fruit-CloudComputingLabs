#ifndef USERSERVER_HPP
#define USERSERVER_HPP
#include "Userall.hpp"
using namespace std;

class UserServer {
public:
    static int fd_Epoll;
    static unordered_map<string,string> config;
    static unordered_map<string,UserCommand> cmdTable;
    static bool isConfigLoad;
    static int MAX_EVENT;

    UserServer();
    ~UserServer();

    bool init(int R_fdEpoll);
    void initCmdTable();
    bool loadConfig();
    bool serverSocket();
    void serverRun();   


    void showServer();
    void showClients();
    void showCmdTable();

    // 返回的是和对面服务器链接的socket
    int connectServer(UserClient* c);
    // 发送消息到对应身份的所有客户端
    void sendMessage2All(string message, string des);
    void sendMessage2One(string message, string name);

    // 身份改变
    void clientTypeChanged(string name, string whatType);
public:
    // 链表还是数组?分开还是轮询?
    UserClient* leaderClient;
    UserClient* curClient;
    unordered_map<string, UserClient*> followerClients;
    UserClient *Clients;

    struct sockaddr_in addr_server;
    string config_path;
    string name;
    int fd_server;
    int port;
    int flag_runDone;
};


#endif