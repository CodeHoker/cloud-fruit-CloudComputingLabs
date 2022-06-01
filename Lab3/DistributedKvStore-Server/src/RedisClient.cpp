#include "all.hpp"


using namespace std;

int RedisClient::fd_Epoll = -1;
int RedisClient::nums_Client = 0;

RedisClient::RedisClient() {
    fdSock = -1;
    name = "unnamedClient";
    serverPort = "?";
    clientType = userClient;

    argc = -1;
    argv.clear();
    cmd = nullptr;

    usedBufferRead = 0;
    usedBufferWrite = 0;

    timeLastChat = 0;
    timeCreated  = 0;
}

RedisClient::~RedisClient() {
    closeClient();
}

void RedisClient::initClient
(RedisServer* server,int fd, const sockaddr_in &rhs_addr) {
    linkServer = server;
    fdSock = fd;
    addr = rhs_addr;
    // ①
    string IP(inet_ntoa(addr.sin_addr));
    serverPort = to_string(ntohs(addr.sin_port));
    name = IP + ":" + serverPort;
    clientType = userClient;// 默认设置为使用客户端,双机的后面再更改
    time(&timeCreated);

    int flag_set = 1;
    len_read = 0;
    setsockopt(fdSock, IPPROTO_TCP, TCP_NODELAY, &flag_set, sizeof(flag_set));
    setsockopt(fdSock, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));
    int opt_val;
    socklen_t opt_len = sizeof(opt_val);
    getsockopt(fdSock, IPPROTO_TCP, TCP_NODELAY, (void*) &opt_val, &opt_len);
    cout << "查看是否禁用了nalge算法" << "   " << opt_val << endl;

    add_event(fd_Epoll, fdSock, true, EP_IN);

    nums_Client++;
}

void RedisClient::closeClient() {
    time_t a;
    time(&a);
    cout << "调用close函数 : " << ctime(&a);
    if(fdSock != -1) {
        cout <<name + " : " << "client close()" << endl;
        delete_event(fd_Epoll, fdSock);
        cmd = nullptr;
        fdSock = -1;
        len_read = -1;
        nums_Client--;
        // 因为也有可能是用户客户端,这时候不改变服务器的个数
        if(clientType != userClient) {
            if(openDebug)
                cout << "有一个no userClient 断开连接" << endl;
            linkServer->numsServer--;
        }
        linkServer->followerClients.erase(name);
        linkServer->userClients.erase(name);
        linkServer->leaderClients.erase(name);
    } else {
        cout << name + " : " <<"unused client" << endl;
    }
}

void RedisClient::showClient() {
    cout << "-------------------------------------" << endl;
    cout << "----------client message-------------" << endl;
    cout << "-------------------------------------" << endl;
    cout <<left<< setw(12) << "fd_Epoll:" << fd_Epoll << endl;
    cout <<left<< setw(12) << "nums_Client:" << nums_Client << endl;
    cout <<left<< setw(12) << "name:" << name << endl;
    cout <<left<< setw(12) << "fd_Client:" << fdSock << endl;
    cout <<left<< setw(12) << "IP addr:" << inet_ntoa(addr.sin_addr) << endl;
    cout <<left<< setw(12) << "port:" << ntohs(addr.sin_port)  << endl;
    cout <<left<< setw(12) << "serverPort:" << serverPort  << endl;
    cout <<left<< setw(12) << "Type:" ;
    if(clientType == userClient)  
        cout << "userClient" << endl;
    else if(clientType == leaderClient)  
        cout << "leaderClient" << endl;
    else if(clientType == followerClient)  
        cout << "followerClient" << endl;
    cout << "-------------------------------------" << endl;
    cout << "-------------------------------------" << endl;
}

void RedisClient::runCommand() {
    string strCmd;
    // cout << "buffer\n" << bufferRead << endl;
    char *cur = strtok(bufferRead, "*");
    while(cur != nullptr) {
        if(openDebug)
            cout << "正在拆分数据包" << endl;
        strCmd = "*";
        strCmd.append(cur);
        
        argv = decodeResp(strCmd.c_str(),strCmd);
        argc = argv.size();
            // cout << "buffer\n" << bufferRead << endl;
        auto it = RedisServer::cmdTable.find(argv[0]);
        if(it == RedisServer::cmdTable.end()) {
            cmd = &(RedisServer::cmdTable["other"]);
        } else {
            cmd = &RedisServer::cmdTable[argv[0]];
        }
        cout <<name + " : " << cmd->name <<" : ";
        for(auto it : argv)
            cout << it << " ";
        cout << endl;
        cmd->proc(this);

        cur = strtok(nullptr, "*");
    }
    // 利用strtok分割掉数据,现在是真的没办法了,并且如果有回复的话也只能回复后面那个,前面的不管了
    if(openDebug)
            cout << "数据包拆分结束,准备送回" << endl;
    // 也有可能有问题,比如set , get , del 等配合onetime使用就不行
    if(argc <= 1 || argv[1] != "onetime") {
        mod_event(fd_Epoll, fdSock, EPOLLOUT);
    } else { 
    // 第二个参数是onetimes的时候就不发送,已经验证过是正确的
        mod_event(fd_Epoll, fdSock, EPOLLIN);
    }
    if(openDebug)
            cout << "是否回送事件设置完毕" << endl;

}

bool RedisClient::clientRead() {
    if(openDebug)
       cout <<name + " : " << "client reading..." << endl;

    len_read = read(fdSock, bufferRead, size_BufferRead);
    bufferRead[len_read]='\0';
    if(len_read == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) 
            return true;
        else 
            return false;
    } else if(len_read == 0) {
        return false;
    }

    // cout <<"clientReading" << bufferRead << endl;

    time(&timeLastChat);
    if(clientType == leaderClient) 
        linkServer->lastChatWithLeader = timeLastChat;
    cout <<name + " : " << ctime(&timeLastChat);
    return true;
}

bool RedisClient::clientWrite() {
    if(openDebug)
        cout <<name + " : " << "client writing..." << endl;
    // 找到了问题所在,len_read没更新,所有发送的数据很少
    int retVal = write(fdSock, bufferWrite, usedBufferWrite);
    // cout << "即将发送\n" <<bufferWrite;
    if(retVal <= 0)
        return false;
    mod_event(fd_Epoll, fdSock, EPOLLIN);
    return true;
}

