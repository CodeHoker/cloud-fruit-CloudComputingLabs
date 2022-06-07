#include "Userall.hpp"
using namespace std;

int UserClient::fd_Epoll = -1;
int UserClient::nums_Client = 0;

UserClient::UserClient() {
    fd_client = argc = -1;
    name = "NoOne_Client";
    serverPort = "?";

    usedBufferRead  = 0;
    usedBufferWrite = 0;
    len_read = 0;

    timeLastChat = 0;
    timeCreated  = 0;
}

UserClient::~UserClient() {
    closeClient();
}

void UserClient::initClient
(UserServer* server,int fd, const sockaddr_in &rhs_addr) {
    linkServer = server;
    fd_client = fd;
    serverAddr = rhs_addr;
    // ①
    string IP(inet_ntoa(rhs_addr.sin_addr));
    serverPort = to_string(ntohs(rhs_addr.sin_port));
    name = IP + ":" + serverPort;
    clientType = followerClient;
    
    time(&timeCreated);

    int flag_set = 1;
    setsockopt(fd_client, IPPROTO_TCP, TCP_NODELAY, &flag_set, sizeof(flag_set));
    setsockopt(fd_client, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));
    // 修改成非一次性接受,是为了能够随时处理设置新的主数据库服务器
    add_event(fd_Epoll, fd_client, false, EP_IN);
    
    nums_Client++;
    showClient();
}

void UserClient::initSTDIN(UserServer* server) {

    linkServer = server;
    serverAddr = server->addr_server;
    serverPort = to_string(server->port);
    fd_client = 0;
    name = "STDIN";
    clientType = STDINClient;
    time(&timeCreated);


    struct  epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd= STDIN_FILENO;
    epoll_ctl(fd_Epoll, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    showClient();
}
void UserClient::closeClient() {
    if(fd_client != -1) {
        cout <<name + " : " << "client close()" << endl;
        delete_event(fd_Epoll, fd_client);
        cmd = nullptr;
        fd_client = -1;
        len_read  = -1;
        nums_Client--;

        if(linkServer->leaderClient && linkServer->leaderClient->name == name) {
            linkServer->leaderClient = nullptr;
        }
        if(linkServer->curClient && linkServer->curClient->name == name) {
            linkServer->curClient = nullptr;
        }
        linkServer->followerClients.erase(name);
    } else {
        cout << name + " : " <<"unused client" << endl;
    }
}

bool UserClient::clientRead() {
    // if(openDebug)
    //    cout <<name + " : " << "client reading..." << endl;

    len_read = read(fd_client, bufferRead, size_BufferRead);
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
    cout <<name + " : " << ctime(&timeLastChat);
    return true;
}

void UserClient::STDINRunCommand() {
    // 1.STDIN的输入,是否有可能出现粘包问题?否定,NO!
    // 2.将buffer里面的数据生成RESP序列
    string str_cmd(bufferRead);
    vector<string> argvFORresp = {"*"};
    argv.clear();
    stringstream ss(str_cmd);
    while(ss >> str_cmd) {
        argvFORresp.emplace_back("$");
        argvFORresp.emplace_back(str_cmd);
        argv.emplace_back(str_cmd);
    }
    argc = argv.size();
    str_cmd = encodeResp(argvFORresp, 0);
    fillBuffer(this, str_cmd);
    //str_cmd保存了生成的RESP序列,用以转发
    // argv保存了已经解析过的参数,用以函数执行

    auto it = UserServer::cmdTable.find(argv[0]);
    if(it == UserServer::cmdTable.end()) {
        cmd = &(UserServer::cmdTable["printf"]);
    } else {
        cmd = &it->second;
    }

    cout <<name + " : " << cmd->name <<" : ";
    for(auto it : argv) 
        cout << it << " ";
    cout << endl;

    if(cmd->type == cmd->Printf || cmd->type == cmd->User)
        cmd->proc(this);
    else if(cmd->type == cmd->TransmitCur) {
        if(linkServer->curClient) 
            write(linkServer->curClient->fd_client, bufferWrite, usedBufferWrite);
        else 
            cout << "未设置curClient,转发失败" << endl;
    } else if(cmd->type == cmd->TransmitLeader) {
        if(linkServer->leaderClient) 
            write(linkServer->leaderClient->fd_client, bufferWrite, usedBufferWrite);
        else  
            cout << "未设置leaderClient,转发失败" << endl;
    } else if(cmd->type == cmd->TransmitFollower) {
        if(!linkServer->followerClients.empty()) 
            write(linkServer->followerClients.begin()->second->fd_client, bufferWrite, usedBufferWrite);
        else {
            cout << "不存在任何一个followerClient,转发失败" << endl;
            cout << "尝试向leaderClient转发..." << endl;
            if(linkServer->leaderClient) 
                write(linkServer->leaderClient->fd_client, bufferWrite, usedBufferWrite);
            else  
                cout << "未设置leaderClient,转发失败" << endl;
        }
    }
} 

void UserClient::runCommand() {

    string strCmd(bufferRead);
    argv = decodeResp(strCmd.c_str(),strCmd);
    argc = argv.size();

    auto it = UserServer::cmdTable.find(argv[0]);
    if(it == UserServer::cmdTable.end()) {
        cmd = &(UserServer::cmdTable["printf"]);
    } else {
        cmd = &it->second;
    }

    cout <<name + " : " << cmd->name <<" : ";
    for(auto it : argv) 
        cout << it << " ";
    cout << endl;

    if(cmd->type == cmd->Printf || cmd->type == cmd->User)
        cmd->proc(this);
    else {
        cout << "接收到了一条未知响应的信息" << endl;
        cout << bufferRead << endl;
    }

}

void UserClient::showClient() {
    cout << "-------------------------------------" << endl;
    cout << "----------client message-------------" << endl;
    cout << "-------------------------------------" << endl;
    cout <<left<< setw(12) << "fd_Epoll:" << fd_Epoll << endl;
    cout <<left<< setw(12) << "nums_Client:" << nums_Client << endl;
    cout <<left<< setw(12) << "name:" << name << endl;
    cout <<left<< setw(12) << "fd_Client:" << fd_client << endl;
    cout <<left<< setw(12) << "IP addr:" << inet_ntoa(serverAddr.sin_addr) << endl;
    cout <<left<< setw(12) << "serverPort:" << serverPort  << endl;
    cout <<left<< setw(12) << "Type:" ;
    if(clientType == leaderClient)  
        cout << "leaderClient" << endl;
    else if(clientType == followerClient)  
        cout << "followerClient" << endl;
    else if(clientType == STDINClient)
        cout << "STDINClient" << endl;
    cout << "-------------------------------------" << endl;
    cout << "-------------------------------------" << endl;
}
