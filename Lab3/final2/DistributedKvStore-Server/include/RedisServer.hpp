#ifndef REDISSERVER_HPP
#define REDISSERVER_HPP
#include "all.hpp"
using namespace std;

class RedisServer {
public:
    static int fd_Epoll;
    static unordered_map<string,string> config;
    static unordered_map<string,RedisCommand> cmdTable;
    static bool isConfigLoad;
    static int MAX_EVENT;

    // 时间事件
    static RedisServer* curServer;
    static void serverCron(int sig);


    RedisServer();
    ~RedisServer();

    bool init(int, string);
    void initCmdTable();
    bool loadConfig();
    bool serverSocket();
    void serverRun();   


    void showServer();
    void showOldCmds();
    void showClients();
    string showCmdTable();

    // 返回的是和对面服务器链接的socket
    int connectServer(RedisClient* c);
    // 发送消息到对应身份的所有客户端
    void sendMessage(string message, string des);
    // 心跳的发送
    string heartBeat_send(string type, bool onetime, int offSet);
    // 将该客户端变成从机身份
    bool clientTypeChanged(RedisClient* c, string whatType);
    // 将自身的身份改变
    void serverTypeChanged(string whatType);
    // 发起选举
    void election();
    // 判断选举是否成功
    void electionResult();
    // 封装选举信息
    string message_Election(string type, int term, string s);
    // 改变其他服务器的身份
    string message_Change(string cmd, string type);
public:
    // 链表还是数组?分开还是轮询?
    unordered_map<string, RedisClient*> userClients;
    unordered_map<string, RedisClient*> leaderClients;
    unordered_map<string, RedisClient*> followerClients;
    RedisClient *Clients;
    


    // 复制积压缓冲区
    vector<string> oldCmds;

    struct sockaddr_in addr_server;
    string config_path;
    string name;
    int fd_server;
    int port;
    enum Type {leaderServer = 0, candidateServer, followerServer};
    Type serverType;

    int flag_runDone;

    // RAFT一致性协议
    int term;
    int tickets;
    int numsServer;
    int time2Candidate;
    int time2Election;
    int election_TimeOut;//投票的超时时间
    time_t whenBecomeCandidate;
    time_t lastChatWithLeader;
    // 选举状态标志：0等待、1成功、-1失败
    int flag_election;

};


#endif