#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP
#include "Userall.hpp"
using namespace std;

class UserServer;

class UserClient {
public:
    static const int size_BufferRead = 1024;
    static const int size_BufferWrite = 1024;
    static int fd_Epoll;
    static int nums_Client;

    UserClient();
    ~UserClient();

    void initClient(UserServer* server, int fd, const sockaddr_in & adr);
    void initSTDIN(UserServer* server);
    void closeClient();
    void showClient();
    void runCommand();
    void STDINRunCommand();
    bool clientRead();
    bool clientWrite();

public:
    UserServer* linkServer;//用于在客户端中操作服务器的值
    struct sockaddr_in serverAddr;
    string serverPort;
    // string serverIP;

    int fd_client;
    string name;
    // 指示对面连接的服务器类型 :主机/从机
    enum Type {STDINClient, leaderClient, followerClient};
    Type clientType;

    int argc;//负数代表超过
    vector<string> argv;
    UserCommand *cmd; 

    char bufferRead[size_BufferRead];
    char bufferWrite[size_BufferWrite];
    int usedBufferRead;
    int usedBufferWrite;
    int len_read;

    time_t timeLastChat;
    time_t timeCreated;

};

#endif