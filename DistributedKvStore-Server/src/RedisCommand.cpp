#include "all.hpp"
using namespace std;

unordered_map<string,string> r;
unordered_map<string,string> RedisCommand::store = r;


RedisCommand::RedisCommand() {
    name = "nullptr";
    arity = 0;
    proc = nullptr;
}

RedisCommand::~RedisCommand() {
    proc = nullptr;
}

void RedisCommand::init
(string n, int nums, void (*ptr)(RedisClient*) ) {
    name = n;
    arity = nums;
    proc = ptr;
}

// 原先的逻辑都有一个问题,那就是错误条件判断不是并列的,而是递进的
// 导致进入错误的函数：比如指令对但是参数不够的情况下,会发生错误
void setCommand(RedisClient *c) {
    string strResult;

    if(c->argc < 3)
        strResult = "-ERROR\r\n"; 
    else if(c->argv[0] != "set" && c->argv[0] != "SET")
        strResult = "-ERROR\r\n";
    else {
        c->linkServer->oldCmds.emplace_back(string(c->bufferRead));
        string str_t(c->argv[2]);
        int pos = 3;
        while(pos < c->argv.size()) {
                str_t.append(" " + c->argv[pos++]);
            }
        RedisCommand::store[c->argv[1]] = str_t;
        strResult = "+OK\r\n";
    }

    fillBuffer(c, strResult);
}

void getCommand(RedisClient *c) {
    string strResult;

    if(c->argc != 2)
        strResult = "-ERROR\r\n"; 
    else if(c->argv[0] != "get" && c->argv[0] != "GET")
        strResult =  "-ERROR\r\n";
    else if(RedisCommand::store.find(c->argv[1]) != RedisCommand::store.end()) {
        vector<string> ret;
        ret.push_back("*");
        stringstream ss(RedisCommand::store[c->argv[1]]);
        string str;
        while(ss >> str) {
            ret.emplace_back("$");
            ret.emplace_back(str);
        }
        strResult =  encodeResp(ret, 0);
    } else
        strResult =  "*1\r\n$3\r\nnil\r\n";

    fillBuffer(c, strResult);
}

void delCommand(RedisClient *c) {
    string strResult;

    if(c->argc <= 1) 
        strResult = "-ERROR\r\n";
    else if(c->argv[0] != "del" && c->argv[0] != "DEL")
        strResult = "-ERROR\r\n";
    else {
        c->linkServer->oldCmds.emplace_back(string(c->bufferRead));
        int cnt = 0;
        for(int i = 1; i < c->argv.size(); i++) {
            auto it = RedisCommand::store.find(c->argv[i]);
            if(it == RedisCommand::store.end())
                continue;
            RedisCommand::store.erase(it);
            cnt++;
        }
        strResult = ":" + to_string(cnt) + "\r\n";
    }

    fillBuffer(c, strResult);
}

void testCommand(RedisClient *c) {
    string strResult = "-MISSING\r\n";
    fillBuffer(c, strResult);
}

void showOldCmdsCommand(RedisClient *c) {
    c->linkServer->showOldCmds();
    string strResult = "+OK\r\n";

    fillBuffer(c, strResult);
}

void flushDbCommand(RedisClient *c) {
    c->linkServer->oldCmds.emplace_back(string(c->bufferRead));

    RedisCommand::store.clear();
    string strResult = "+OK\r\n";

    fillBuffer(c, strResult);
}

void showClientsCommand(RedisClient* c) {
    c->linkServer->showClients();
    string strResult = "+OK\r\n";

    fillBuffer(c, strResult);
}

void showCmdTableCommand(RedisClient* c) {
    string strResult = c->linkServer->showCmdTable();
    strResult = "+OK\r\n";

    fillBuffer(c, strResult);
}

void slaveofCommand(RedisClient* c) {
    string strResult;
    int sock_server = c->linkServer->connectServer(c);
    
    if(sock_server <= 0)
        strResult = "-ERROR\r\n";
    else {
        // 确实是发送的对象有问题,不知道为什么只能加密解密后发送,而不能直接发送
        // vector<string> vect = {"*", "$", "beMyMaster", "$", "onetime"};
        string str = c->linkServer->message_Change("beMy","leader");
        write(sock_server, str.c_str(), str.length());
        strResult = "+OK\r\n";
    }

    fillBuffer(c, strResult);
}

void fillBuffer(RedisClient* c, string strResult) {
    memset(c->bufferWrite,'\0',c->usedBufferRead);//那就先初始化
    c->usedBufferWrite = strResult.length();
    strcpy(c->bufferWrite, strResult.c_str());
    // cout << "缓冲区填充：\n" << c->bufferRead << endl; 
}

// ACK/SYN + onetime + offset + data
void ackResponseCommand(RedisClient* c) {
    RedisServer* s = c->linkServer;
    string message;

    if(c->argc < 2 || c->clientType == RedisClient::Type::userClient) {
        message = "-ERROR\r\n";
    } else {
        if(c->argv[1] == "onetime" || stoi(c->argv[2]) == s->oldCmds.size()) { 
            message = s->heartBeat_send("SYN", true, s->oldCmds.size());
        } else {
            message = s->heartBeat_send("SYN", false, (stoi(c->argv[2])+1));
        }
    }

    fillBuffer(c, message);
}
void synResponseCommand(RedisClient* c) {
    // 这个函数还需要更新自己的数据库,不只是发送报文啊
    RedisServer *s = c->linkServer;
    string message;
    if(c->argc < 2 || c->clientType == RedisClient::Type::userClient)
        message = "-ERROR\r\n";
    else {
        if(stoi(c->argv[2]) == s->oldCmds.size()) {
        // 发现问题了,argc[2]是一个偏移量,但是原来对比的使用不是比较便宜量
            // 代表不需要同步
            message = s->heartBeat_send("ACK", true, s->oldCmds.size());
        } else if (stoi(c->argv[2]) == s->oldCmds.size()+1){
            // 需要同步,SYN TIMES OFFSET DEL K1
            c->argv.assign(c->argv.begin()+3, c->argv.end());
            c->cmd = &RedisServer::cmdTable[c->argv[0]];
            c->cmd->proc(c);
            message = s->heartBeat_send("ACK", false, s->oldCmds.size());
            cout <<c->name + " : " << c->cmd->name <<" : ";
            for(auto it : c->argv)
                cout << it << " ";
            cout << endl;
        } else {
            message = s->heartBeat_send("ACK", false, s->oldCmds.size());
        }
    }
    fillBuffer(c, message);
}

// apply/vote onetime/times term yes/no
void applyResponseCommand(RedisClient* c) {
    RedisServer *s = c->linkServer;
    string message;
    if(debugElection) {
        cout << "一个投票申请到达" << endl;
        cout << "申请投票的轮数" << c->argv[2] << endl;
        cout << "当前的轮数" << s->term << endl;
    }
    if(stoi(c->argv[2]) > s->term) {
        if(debugElection)   
            cout << "OK选票发送" << endl;
        s->term = stoi(c->argv[2]);
        message = s->message_Election("VOTE", s->term, "YES");
    } else {
        if(debugElection)   
            cout << "NO选票发送" << endl;
        message = s->message_Election("VOTE", s->term, "NO");
    }

    fillBuffer(c, message);
}

void voteResponseCommand(RedisClient* c) {
    RedisServer *s = c->linkServer;
    string message;
    if(c->argv[3] == "YES" && stoi(c->argv[2]) == s->term)
        s->tickets++;

    fillBuffer(c, message);
}

// beMy onetime type port
void beMyCommand(RedisClient* c) {
    RedisServer* s = c->linkServer;
    string strResult;
    c->serverPort = c->argv[3];
    // 一开始连接的时候,还是用户客户端,转变的时候+1
    if(c->clientType == RedisClient::Type::userClient) {
        s->numsServer++;
        if(openDebug) {
            cout << "beMyCommand,numsServer : " << s->numsServer << endl;
        }   
    }
    if(c->argv[2] == "leader") {
        c->showClient();
        s->serverTypeChanged("leader");
        c->showClient();
        s->clientTypeChanged(c, "follower");
        // 为了解决粘包问题,试试先发送消息,然后过些时间再一致性发送
        // 没用,因为并不是直接发送,而是在write里面发送
        vector<string> ret = {"*","$","otherFollower","$","onetime","$"};
        // 去除掉刚连接的客户端本身
        ret.emplace_back(to_string(s->followerClients.size()-1));
        for(auto it : s->followerClients) {
            if(c == it.second)
                continue;
            ret.emplace_back("$");
            ret.emplace_back(inet_ntoa(it.second->addr.sin_addr));
            ret.emplace_back("$");
            ret.emplace_back(it.second->serverPort);
        }
        strResult = encodeResp(ret, 0);

    } else if(c->argv[2] == "follower") {
        // 应该只有leader回发这个消息吧？
        s->lastChatWithLeader = c->timeLastChat;
        s->serverTypeChanged("follower");
        if(debugElection)
            cout << "服务器身份改变" << endl;
        // 主服务器主动断开的话,这里已经没有了
        // 只是连接不到的话,也很麻烦
        if(!s->leaderClients.empty())
            s->clientTypeChanged(s->leaderClients.begin()->second, "follower");
        if(debugElection)
            cout << "原主服务器身份改变" << endl;
        s->clientTypeChanged(c, "leader");
        if(debugElection)
            cout << "连接的客户端身份改变" << endl;
    } else if(c->argv[2] == "otherfollower") {
        s->serverTypeChanged("follower");
        s->clientTypeChanged(c, "follower");
    }

    fillBuffer(c, strResult);
}

// otherFollower onetime size ip port ip port
void otherFollowerCommand(RedisClient* c) {
    RedisServer* s = c->linkServer;
    string strResult;
    // 特殊情况,如果这个size是0的时候呢？
    int size_otherFollowers = stoi(c->argv[2]);
    int sock_server = -1;
    for(int i = 0; i < size_otherFollowers; i++) {
        c->argv[1] = c->argv[3+2*i];
        c->argv[2] = c->argv[4+2*i];
        // cout << c->argv[3+2*size_otherFollowers] << "??:??" << c->argv[4+2*size_otherFollowers] << endl;
        // cout << c->argv[1] << ":" <<c->argv[2] << endl;
        sock_server = s->connectServer(c);
        if(sock_server > 0) {
            string str  = s->message_Change("beMy","otherfollower");
            s->clientTypeChanged(&s->Clients[sock_server], "follower");
            write(sock_server, str.c_str(), str.length());
        }
    }
    c->argv[1] = "onetime";//不回复了
}

void showServerCommand(RedisClient *c) {
    c->linkServer->showServer();
}