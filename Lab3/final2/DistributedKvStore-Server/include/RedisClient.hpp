#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP
#include "all.hpp"
using namespace std;
class RedisServer;

class RedisClient {
public:
    static const int size_BufferRead = 1024;
    static const int size_BufferWrite = 1024;
    static int fd_Epoll;
    static int nums_Client;

    RedisClient();
    ~RedisClient();

    void initClient(RedisServer*, int , const sockaddr_in &);
    void closeClient();
    void showClient();
    void runCommand();
    bool clientRead();
    bool clientWrite();

public:
    int fdSock;
    struct sockaddr_in addr;
    string name;
    enum Type {userClient = 0, leaderClient, followerClient};
    Type clientType;

    int argc;//负数代表超过
    vector<string> argv;
    RedisCommand *cmd; 

    char bufferRead[size_BufferRead];
    char bufferWrite[size_BufferWrite];
    int usedBufferRead;
    int usedBufferWrite;
    int len_read;
    string serverPort;

    RedisServer* linkServer;//用于在客户端中操作服务器的值
    //代表这个客户端距离上次发送消息已经是多久之前了
    //用以服务器断开未连接的客户端;没有心跳的从客户端/主客户端
    time_t timeLastChat;
    time_t timeCreated;

    // database *curDatabase,指向的数据库指针
    // 以后封装可能会用到,现在就使用一个数据库好了
};

#endif