#ifndef REDISCOMMAND_HPP
#define REDISCOMMAND_HPP
#include "all.hpp"
using namespace std;

class RedisClient;

class RedisCommand {
public:
    static unordered_map<string,string> store;
    RedisCommand();
    ~RedisCommand();
    void init(string n, int, void (*ptr)(RedisClient*) );
public:
    string name;
    int arity;
    void (*proc)(RedisClient*);
};

// 指令表
    // set指令
void setCommand(RedisClient *);
    // get指令
void getCommand(RedisClient *);
    // del指令
void delCommand(RedisClient *);
    // missing指令
void testCommand(RedisClient *);
    // 显示积压缓冲区
void showOldCmdsCommand(RedisClient *c);
    // 清空数据库
void flushDbCommand(RedisClient *c);
    // 展示所有相连的客户端
void showClientsCommand(RedisClient *c);
void showServerCommand(RedisClient *c);
    // 显示可支持的命令
void showCmdTableCommand(RedisClient* c);
    // 主从来了
    // 成为某服务器的从机
void slaveofCommand(RedisClient* c);
    // 和某服务器成为相互之间的从机
void otherFollowerCommand(RedisClient* c);
    // 将来源客户端设为从机客户端
void beMyMasterCommand(RedisClient* c);
    // 填充缓冲区
void fillBuffer(RedisClient* c, string str);
    // 心跳机制ACK/SYN
void ackResponseCommand(RedisClient* c);
void synResponseCommand(RedisClient* c);
void applyResponseCommand(RedisClient* c);
void voteResponseCommand(RedisClient* c);
    // 将这个客户端设为什么身份
void beMyCommand(RedisClient* c);
#endif