#include "all.hpp"
// 提示重复定义的问题,好像通过all.hpp引用没用
// 必须直接引用RedisServer.hpp
// 再进一步,是和all中的引用顺序有关,能够直接引用解决
using namespace std;

bool RedisServer::isConfigLoad = false;
int RedisServer::fd_Epoll = -1;
int RedisServer::MAX_EVENT = 64;
unordered_map<string,string> m;//帮助初始化
unordered_map<string,string> RedisServer::config = m;
unordered_map<string,RedisCommand> m2;
unordered_map<string,RedisCommand> RedisServer::cmdTable = m2;
RedisServer* RedisServer::curServer = nullptr;
// 类的非常量静态对象需要先初始化才能使用,
// 否则出现undefined reference to


RedisServer::RedisServer() :
    config_path("../config/config.txt"),
    fd_server(-1), port(-1), name("unknown"),
    serverType(leaderServer) {
    flag_runDone = term = time2Candidate = time2Election = tickets = 0;
    numsServer = 1; //因为一开始自己也算作是一个
    flag_election = -1;//一开始的就是选举失败,方便发起选举
    // 由于serverRun弄了个sleep,所以选举没那么快,所以把选举超时时间拉长
    election_TimeOut = 10;
    if(openDebug)
        cout << "RedisServer() done" << endl;
}

RedisServer::~RedisServer() {
    for(int i = 0;i < MAX_EVENT; i++) {
        Clients[i].closeClient();
    }
    oldCmds.clear();
    userClients.clear();
    leaderClients.clear();
    followerClients.clear();
    if(openDebug)
        cout << "~RedisServer() done" << endl;
}

bool RedisServer::loadConfig() {
    fstream file_config;
    file_config.open(config_path);//打开文件	
    if(!file_config.is_open()) {
        perror("config open");
        return false;
    }

    char tmp[1000];
    while(!file_config.eof()) {//循环读取每一行 
        file_config.getline(tmp,1000);//每行读取前1000个字符，1000个应该足够了
        string line(tmp);
        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
            if(pos == string::npos) return false;
        string key   = line.substr(0, pos-1);//取=号之前
        string value = line.substr(pos+2);//取=号之后
        config[key] = value;
    }
    // for(auto it = config.begin(); it != config.end(); it++) 
    //     cout << it->first << " " << it ->second << endl;
    
    return true;
}

bool RedisServer::serverSocket() {
    //socket()
    fd_server = socket(PF_INET, SOCK_STREAM, 0);
    if(fd_server == -1) {
        handler_error("socket()");
        return false;
    }

    // 设置端口复用
    int flag_set = 1;
    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));

    //bind()需要<arpa/inet.h>
    addr_server.sin_family = AF_INET;
    port = stoi(config["serverPort"]);
    addr_server.sin_port = htons(port);
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd_server, (struct sockaddr*)&addr_server, sizeof(addr_server));

        //listen()
    listen(fd_server, 10);

    return true;
}

void RedisServer::showServer() {
    cout << "-------------------------------------" << endl;
    cout << "----------server message-------------" << endl;
    cout << "-------------------------------------" << endl;
    cout <<left<< setw(12) << "config_path:" << config_path << endl;
    cout <<left<< setw(12) << "fd_Epoll:" << fd_Epoll << endl;
    cout <<left<< setw(12) << "fd_server:" << fd_server << endl;
    cout <<left<< setw(12) << "port:" << port << endl;
    cout <<left<< setw(12) << "name:" << name << endl;
    cout <<left<< setw(12) << "term:" << term << endl;
    cout <<left<< setw(12) << "numsServer:" << numsServer << endl;
    cout <<left<< setw(12) << "Type:" ;
    if(serverType == leaderServer)  
        cout << "leaderServer" << endl;
    else if(serverType == candidateServer)  
        cout << "candidateServer" << endl;
    else if(serverType == followerServer)  
        cout << "followerServer" << endl;
    cout << "-------------------------------------" << endl;
    cout << "-------------------------------------" << endl;
}

bool RedisServer::init(int fd_epoll, string strName) {
    // 需要在类中将标志设置为否
    if(!isConfigLoad) {
        loadConfig();
        isConfigLoad = true;
    }
    serverSocket();
    RedisClient::fd_Epoll = fd_epoll;
    fd_Epoll = fd_epoll;
    name = strName;
    serverType = leaderServer;//默认为领导者服务器
    initCmdTable();
    // showCmdTable();

    add_event(fd_epoll, fd_server, false, EP_IN);
    cout <<name+" : "<< "Redis server is ready" << endl;
    showServer();


    return true;
}

// 循环函数等到客户端结束之后再修正
void RedisServer::serverRun() {
    epoll_event events[MAX_EVENT];
    Clients = new RedisClient[MAX_EVENT];
    RedisServer::curServer = this;
    srand(time(0));
    setAlarm(2);
    set_signal(SIGALRM,RedisServer::serverCron);

    while(!flag_runDone) {
        // sleep(2);
        // cout << name+" : "  <<"waiting..." << endl;
        int num = epoll_wait(fd_Epoll, events, MAX_EVENT, -1);
        //阻塞时被信号中断后返回错误
        if((num < 0) && (errno != EINTR)) {
            handler_error("epoll_wait()");
        }if(openDebug)
            cout << name+" : "  <<"something is comming" << endl;

        for(int i = 0; i < num ; i++) {
            int fd_event = events[i].data.fd;
            int fd_client;
            if(fd_event == fd_server) {
                //服务器监听套接字响应，有新的客户端连接
                if(openDebug) {
                    cout << name+" : "  <<"client connected" << endl;
                }
                struct sockaddr_in addr_client;
                socklen_t len_addr = sizeof(addr_client); 
                fd_client = accept(fd_server, (struct sockaddr*)&addr_client, &len_addr);
                if(RedisClient::nums_Client >= MAX_EVENT) {
                    close(fd_client);
                    continue;
                }
                srand(rand() % addr_client.sin_port);//更新随机数种子
                Clients[fd_client].initClient(this, fd_client, addr_client); 
                userClients.emplace(Clients[fd_client].name, &Clients[fd_client]);     
                Clients[fd_client].showClient();
            } else if(events[i].events & EPOLLIN) {
                if(Clients[fd_event].clientRead()) {
                    Clients[fd_event].runCommand();
                } else {
                    Clients[fd_event].closeClient();
                }
            } else if(events[i].events & EPOLLOUT) {
                Clients[fd_event].clientWrite();
            }
        }
    }
}

void RedisServer::initCmdTable() {
    RedisCommand *cur;
    
    cur = new RedisCommand();
    cur->init("other", -1, testCommand);
    cmdTable["other"] = *cur;

    cur = new RedisCommand();
    cur->init("set", -3, setCommand);
    cmdTable["set"] = *cur;

    cur = new RedisCommand();
    cur->init("get", 2, getCommand);
    cmdTable["get"] = *cur;

    cur = new RedisCommand();
    cur->init("del", -2, delCommand);
    cmdTable["del"] = *cur;

    cur = new RedisCommand();
    cur->init("showOldCmds", -1, showOldCmdsCommand);
    cmdTable["showOldCmds"] = *cur;

    cur = new RedisCommand();
    cur->init("flushDb", -1, flushDbCommand);
    cmdTable["flushDb"] = *cur;
    
    cur = new RedisCommand();
    cur->init("showClients", -1, showClientsCommand);
    cmdTable["showClients"] = *cur;    

    cur = new RedisCommand();
    cur->init("showServer", -1, showServerCommand);
    cmdTable["showServer"] = *cur;  

    cur = new RedisCommand();
    cur->init("showCmdTable", -1, showCmdTableCommand);
    cmdTable["showCmdTable"] = *cur;

    
    cur = new RedisCommand();
    cur->init("slaveof", -1, slaveofCommand);
    cmdTable["slaveof"] = *cur;

    cur = new RedisCommand();
    cur->init("SYN", -1, synResponseCommand);
    cmdTable["SYN"] = *cur;

    cur = new RedisCommand();
    cur->init("ACK", -1, ackResponseCommand);
    cmdTable["ACK"] = *cur;

    cur = new RedisCommand();
    cur->init("APPLY", -1, applyResponseCommand);
    cmdTable["APPLY"] = *cur;

    cur = new RedisCommand();
    cur->init("VOTE", -1, voteResponseCommand);
    cmdTable["VOTE"] = *cur;

    
    cur = new RedisCommand();
    cur->init("beMy", -1, beMyCommand);
    cmdTable["beMy"] = *cur;

    cur = new RedisCommand();
    cur->init("otherFollower", -1, otherFollowerCommand);
    cmdTable["otherFollower"] = *cur;
}

string RedisServer::showCmdTable() {
    string ret;
    int cnt = 1;
    for(auto it = cmdTable.begin(); it != cmdTable.end(); it++) {
        ret.append(it->first + " ");
        cout<< name + " UsefulCmds: "<< setw(3) << cnt++ << it->first << endl;
    }

    return ret;
}

void RedisServer::showOldCmds() {
    int cnt = 1;
    for(auto it = oldCmds.begin(); it != oldCmds.end(); it++)
        cout<< name + " OldCmds: " << setw(4) << cnt++ <<"\n"<< *it;
}

void RedisServer::showClients() {
    cout << "-------------------------------------" << endl;
    cout<< name + " : User Clients"<< endl;
    cout << "-------------------------------------" << endl;
    for(auto it : userClients)
        it.second->showClient();
        
    cout << "-------------------------------------" << endl;
    cout<< name + " : Leader Clients"<< endl;
    cout << "-------------------------------------" << endl;
    for(auto it : leaderClients)
        it.second->showClient();

    cout << "-------------------------------------" << endl;
    cout<< name + " : Follower Clients"<< endl;
    cout << "-------------------------------------" << endl;
    for(auto it : followerClients)
        it.second->showClient();
}

int RedisServer::connectServer(RedisClient* c) {
    // socket() 
    if(c->argc < 3)
        return -1;
	int sockfd_client;
	sockfd_client = socket(PF_INET, SOCK_STREAM, 0); 
    if(sockfd_client == -1) {
        perror("socket()");
        return -1;
    }

    //connect()
	struct sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr)); 
	serv_adr.sin_family=AF_INET;
    // cout << c->argv[1] << ":" <<c->argv[2] << endl;
	serv_adr.sin_addr.s_addr=inet_addr(c->argv[1].c_str());
	serv_adr.sin_port=htons(atoi(c->argv[2].c_str()));
	 
	if(connect(sockfd_client, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        perror("connect()");
        return -1;
    }

    // 问题在于每个连接的客户端名字都是一样的,造成了覆盖
    Clients[sockfd_client].initClient(this, sockfd_client, serv_adr);
    Clients[sockfd_client].clientType = RedisClient::Type::leaderClient;
    time(&Clients[sockfd_client].timeLastChat);//不初始化的话,一下子就变成候选者了
    serverTypeChanged("follower");
    leaderClients.emplace(Clients[sockfd_client].name, &Clients[sockfd_client]);  

    numsServer++;//连接上一个新的服务器 
    return sockfd_client;  
}

bool RedisServer::clientTypeChanged(RedisClient* c, string whatType) {
    if(c == nullptr)//CNM的,c == nullptr写成 c = nullptr
        return true;
    if(whatType == "follower") {
        Clients[c->fdSock].clientType = RedisClient::Type::followerClient;
        userClients.erase(c->name);
        leaderClients.erase(c->name);
        followerClients.emplace(c->name, &Clients[c->fdSock]);
    } else if(whatType == "leader") {
        Clients[c->fdSock].clientType = RedisClient::Type::leaderClient;
        userClients.erase(c->name);
        followerClients.erase(c->name);
        leaderClients.emplace(c->name, &Clients[c->fdSock]);
    } 
    return true;
}


// 时间事件部分

void RedisServer::sendMessage(string message, string des) {
    unordered_map<string, RedisClient*>::iterator it, end;
    if(des == "leader") {
        it = leaderClients.begin();
        end = leaderClients.end();
    } else if(des == "user") {
        it = userClients.begin();
        end = userClients.end();
    } else if(des == "follower") {
        it = followerClients.begin();
        end = followerClients.end();
    } else 
        return;

    while(it != end) {
        int i = 10;
        string str;
        // while(i--) {
        //     // 检验出来,关闭了Nagle算法,但是由于接收方接受的太慢了
        //     write(it->second->fdSock, message.c_str(), message.length());
        // }
        write(it->second->fdSock, message.c_str(), message.length());
        it++;
    }

}

void RedisServer::serverCron(int sig) {
    time_t now;
    time(&now);
    if(sig != SIGALRM)  
        return;
    if(curServer->serverType == RedisServer::Type::leaderServer) {
        // 主机
        curServer->sendMessage(curServer->heartBeat_send("SYN", false, curServer->oldCmds.size()), "follower");
    } 
    else if(curServer->serverType == RedisServer::Type::followerServer) {
        // 从机,有个问题,就是主机直接断开的时候,这个begin就不存在了,怎么判断
        if(now - curServer->lastChatWithLeader >= curServer->time2Candidate) {
            curServer->serverTypeChanged("candidate");
            // 可能是找到原因了
            if(!curServer->leaderClients.empty())
                curServer->leaderClients.begin()->second->closeClient();//主机断开
            cout << "变成候选者" << endl;
        }
    } else if(curServer->serverType == RedisServer::Type::candidateServer) {
    //     // 候选者
        if(now - curServer->whenBecomeCandidate >= curServer->time2Election) {
            cout << "发起投票或者等待投票结果" << endl;
            curServer->election();
        }
    }
}

string RedisServer::heartBeat_send(string type, bool onetime, int offSet) {
    // ACK/SYN + onetime + offset + data
    vector<string> vec = {"*", "$", type, "$"};
    if(onetime)
        vec.emplace_back("onetime");
    else 
        vec.emplace_back("times");
    vec.emplace_back("$");
    if(offSet > oldCmds.size())
            offSet = oldCmds.size();
    vec.emplace_back(to_string(offSet));
    
    offSet--;
    if(type == "SYN" && offSet >= 0) {

        int pos = -1, cnt = 0;
        for(int i = 0; i < oldCmds[offSet].length(); i++) {
            if(oldCmds[offSet][i] == '$' && pos == -1)
                pos = i;
            if(oldCmds[offSet][i] == '$')
                cnt ++;
        } 
        int cnt2 = cnt;
        while(cnt2--) {
            vec.emplace_back("$");
            vec.emplace_back("?");
        }
        string str1 = encodeResp(vec,0);
        str1 = str1.substr(0, str1.length()-7*cnt);
        string str2 = oldCmds[offSet].substr(pos);
        return str1.append(str2);

    } else if(type == "ACK" || offSet < 0) {
        // for(auto it : vec)
        //     cout << it <<" ";
        // cout << endl;
        return encodeResp(vec, 0);
    }
    else 
        return "-ERROR\r\n";
}

void RedisServer::serverTypeChanged(string whatType) {
    if(whatType == "candidate") {
        serverType = Type::candidateServer;
        // 不知道是不是随机范围太小了,导致两个人的选举时间
        time2Election = rand()%10;
        if(debugElection)
            cout << "投票发起的时间" << time2Candidate << endl;
        time(&whenBecomeCandidate);
    } else if(whatType == "leader") {
        serverType = leaderServer;
    } else if(whatType == "follower") {
        serverType = followerServer;
        time2Candidate = 3 + rand()%5; //设置变成候选者的时间
    }
}

void RedisServer::election() {
    if(flag_election == -1) {//只有选举失败的时候,才更新选票 
        term++;//把term的改变放到选票时间里面,否则term会一直一致,和随机数无关
        tickets = 1;//自己给自己投票,不能用++,每轮投票的时候都重置票数
        flag_election = 0;//每次发起投票都要将状态设为等待中
        sendMessage(message_Election("APPLY", term, "nothing"), "leader");
        sendMessage(message_Election("APPLY", term, "nothing"), "follower");
        if(debugElection)
            cout << "开启新的投票" << endl;
    } else 
        if(debugElection)
            cout << "等待投票结果" << endl;

    electionResult();//用来改变flag_election的状态
    if(flag_election == 1) {
        // 选举成功
        cout << "选举成功" << endl;
        cout << "服务器身份改变" << endl;
        serverTypeChanged("leader");
        cout << "向所有从机确认身份" << endl;
        sendMessage(message_Change("beMy", "follower"), "follower");
        sendMessage(message_Change("beMy", "follower"), "user");
        // 原来的那个主客户端怎么处理？

    } else if(flag_election == -1){
        // 选举失败
        if(debugElection)
            cout << "选举失败" << endl;
        serverTypeChanged("candidate");
    } else if (flag_election == 0) {
        if(debugElection)
            cout << "还需要等待选举结果" << endl;
    }
}

string RedisServer::message_Election(string type, int term, string s) {
    vector<string> vec = {"*", "$", type, "$", "onetime", "$", to_string(term), "$", s};
    if(type == "APPLY") 
        vec[4] = "times";
    
    return encodeResp(vec, 0);
}

string RedisServer::message_Change(string cmd, string type) {
    vector<string> vec = {"*", "$", cmd, "$", "onetime", "$", type, "$", to_string(port)};
    // 将自己的主机编号也给对面发送过去
    // 如果发送的是成为从机,那么需要接受一个代表其他从机的消息段
    if(type == "leader")
        vec[4] = "times";
    return encodeResp(vec, 0);
}

void RedisServer::electionResult() {
    time_t now;
    time(&now);
    int timeRest = election_TimeOut - (now - whenBecomeCandidate);
    // 阻塞等待选票结果
    if(timeRest > 0) {
        cout << "当前票数" <<tickets << endl;
        cout << "当前服务器个数" << numsServer << endl;
        cout << "当前轮数" << term << endl;
        if(tickets > numsServer/2) {
            flag_election = 1;//代表选举成功
            if(debugElection)
                cout << "选举成功" << endl;
        } else {
            flag_election = 0;//代表还需要等待
            if(debugElection) 
                cout << "result 还需要等待" << endl;
        }
    } else 
        flag_election = -1;//代表选举失败
}
