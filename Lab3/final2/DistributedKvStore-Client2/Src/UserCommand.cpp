#include "Userall.hpp"
using namespace std;

void UserCommand::init(string n, void (*ptr)(UserClient*)) {
    name = n;
    proc = ptr;
}

void connectDBServerCommand (UserClient* c) {
    UserServer *s = c->linkServer;
    if(s->connectServer(c) > 0)
        cout << "连接数据库服务器成功" << endl;
    else 
        cout << "连接数据库服务器失败" << endl;
}

void printfCommand(UserClient *c) {
    cout << c->bufferRead;
}

void userShowClientsCommand(UserClient *c) {
    c->linkServer->showClients();
}
void userShowServerCommand(UserClient *c) {
    c->linkServer->showServer();
}

void userShowCmdTableCommand(UserClient* c) {
    c->linkServer->showCmdTable();
}

void fillBuffer(UserClient* c, string strResult) {
    memset(c->bufferWrite,'\0',c->usedBufferWrite);//那就先初始化
    c->usedBufferWrite = strResult.length();
    strcpy(c->bufferWrite, strResult.c_str());
    // cout << "缓冲区填充：\n" << c->bufferRead << endl; 
}

void userSetLeaderCommand(UserClient *c) {
    UserServer* s = c->linkServer;
    if(c->argc < 2)
        cout << "-Error" << endl;
    else
        s->clientTypeChanged(c->argv[1], "leader");
}

void userSetCurCommand(UserClient *c) {
    UserServer* s = c->linkServer;
    if(c->argc < 2)
        cout << "-Error" << endl;
    else
        s->clientTypeChanged(c->argv[1], "cur");
}

void beMyCommand(UserClient *c) {
    // 尽量不去修改已经完成的服务器结构
    // beMy onttime follower
    c->argv[0] = "setLeader";
    c->argv[1] = c->name;
    userSetLeaderCommand(c);
}

void loadConfigCommand(UserClient *c) {
    string file_path;
    // 默认路径
    if(c->argc < 2) 
        file_path = "./config/servers.txt";
     else  
        file_path = c->argv[1];
        
    fstream file_config;
    file_config.open(file_path);//打开文件	
    if(!file_config.is_open()) 
        perror("config open");
    
    char tmp[1000];
    while(!file_config.eof()) {//循环读取每一行 
        file_config.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
        string line(tmp);
        // 以!开始的标注,不进行读取
        if(line[0] == '!')
            continue;

        memset(c->bufferRead,'\0',c->usedBufferRead);//那就先初始化
        c->usedBufferRead = line.length();
        strcpy(c->bufferRead, line.c_str()); 
        c->STDINRunCommand();
    }
}
