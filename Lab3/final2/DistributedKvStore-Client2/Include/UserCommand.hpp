#ifndef USERCOMMAND_HPP
#define USERCOMMAND_HPP
#include "Userall.hpp"
using namespace std;

class UserClient;

class UserCommand {
public:
    UserCommand() {}
    ~UserCommand() {}
    void init(string n, void (*ptr)(UserClient*));
public:
    string name;
    void (*proc)(UserClient*);
    // 指示怎么处理这条命令
    enum commandType {User = 0, TransmitCur, TransmitLeader, TransmitFollower, Printf};
    commandType type;
};

void fillBuffer(UserClient* c, string strResult);
// 指令表
void connectDBServerCommand (UserClient *c);
void printfCommand(UserClient *c);
void userShowClientsCommand(UserClient *c);
void userShowServerCommand(UserClient *c);
void userShowCmdTableCommand(UserClient* c);
void userSetLeaderCommand(UserClient *c);
void userSetCurCommand(UserClient *c);
void beMyCommand(UserClient *c);
void loadConfigCommand(UserClient *c);
#endif