#include "Userall.hpp"
using namespace std;

int UserServer::fd_Epoll = -1;
unordered_map<string,string> m1;
unordered_map<string,UserCommand> m2;
unordered_map<string,string> UserServer::config       = m1;
unordered_map<string,UserCommand> UserServer::cmdTable = m2;
bool UserServer::isConfigLoad = false;
int UserServer::MAX_EVENT = 16;

UserServer::UserServer() {
    config_path = "./config/config.txt";
    name = "UserServer";
    fd_server = port = -1;
    flag_runDone = false; 
    leaderClient = curClient = nullptr;
}

UserServer::~UserServer() {
    for(int i = 0;i < MAX_EVENT; i++) {
        Clients[i].closeClient();
    }
    
    if(leaderClient)
        delete leaderClient;
    if(curClient)
        delete curClient;

    followerClients.clear();
}

bool UserServer::loadConfig() {
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
        // 以!开始的标注,不进行读取
        if(line[0] == '!')
            continue;
        size_t pos = line.find('=');//找到每行的“=”号位置，之前是key之后是value
            if(pos == string::npos) return false;
        string key   = line.substr(0, pos-1);//取=号之前
        string value = line.substr(pos+2);//取=号之后
        config[key] = value;
    }
    for(auto it = config.begin(); it != config.end(); it++) 
        cout << it->first << " " << it ->second << endl;
    
    return true;
}

bool UserServer::serverSocket() {
    //socket()
    fd_server = socket(PF_INET, SOCK_STREAM, 0);
    if(fd_server == -1) {
        perror("socket()");
        return false;
    }

    // 设置端口复用
    int flag_set = 1;
    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));

    //bind()需要<arpa/inet.h>
    addr_server.sin_family = AF_INET;
    port = stoi(config["UserServerPort"]);
    addr_server.sin_port = htons(port);
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd_server, (struct sockaddr*)&addr_server, sizeof(addr_server));
    this->addr_server = addr_server;

        //listen()
    listen(fd_server, 10);

    return true;
}

bool UserServer::init(int r_fdEpoll) {
    // 需要在类中将标志设置为否
    if(!isConfigLoad) {
        loadConfig();
        isConfigLoad = true;
    }
    serverSocket();
    UserServer::fd_Epoll = r_fdEpoll;
    UserClient::fd_Epoll = r_fdEpoll;//没初始化,不能使用
    initCmdTable();

    add_event(fd_Epoll, fd_server, false, EP_IN);
    // if(openDebug)
    //     cout <<name + " : "<< "user server is ready" << endl;
    // showServer();
    showServer();
    return true;
}

void UserServer::initCmdTable() {
    UserCommand *cur = new UserCommand();

    cur->init("printf", printfCommand);
    cur->type = cur->Printf;
    cmdTable["printf"] = *cur;

    cur->init("connect", connectDBServerCommand);
    cur->type = cur->User;
    cmdTable["connect"] = *cur;

    cur->init("beMy", beMyCommand);
    cur->type = cur->User;
    cmdTable["beMy"] = *cur;

    cur->init("loadConfig", loadConfigCommand);
    cur->type = cur->User;
    cmdTable["loadConfig"] = *cur;

    cur->init("userShowClients", userShowClientsCommand);
    cur->type = cur->User;
    cmdTable["userShowClients"] = *cur;

    cur->init("userShowServer", userShowServerCommand);
    cur->type = cur->User;
    cmdTable["userShowServer"] = *cur;

    cur->init("userSetLeader", userSetLeaderCommand);
    cur->type = cur->User;
    cmdTable["setLeader"] = *cur;

    cur->init("userShowServer", userSetCurCommand);
    cur->type = cur->User;
    cmdTable["setCur"] = *cur;

    cur->init("userShowCmdTable", userShowCmdTableCommand);
    cur->type = cur->User;
    cmdTable["userShowCmdTable"] = *cur;

    // 不拥有实际的函数,指示一种转发
    cur->init("set", nullptr);
    cur->type = cur->TransmitLeader;
    cmdTable["set"] = *cur;

    cur->init("del", nullptr);
    cur->type = cur->TransmitLeader;
    cmdTable["del"] = *cur;

    cur->init("get", nullptr);
    cur->type = cur->TransmitFollower;
    cmdTable["get"] = *cur;

    cur->init("flushDb", nullptr);
    cur->type = cur->TransmitLeader;
    cmdTable["flushDb"] = *cur;

    cur->init("showOldCmds", nullptr);
    cur->type = cur->TransmitCur;
    cmdTable["showOldCmds"] = *cur;

    cur->init("showClients", nullptr);
    cur->type = cur->TransmitCur;
    cmdTable["showClients"] = *cur;

    cur->init("showServer", nullptr);
    cur->type = cur->TransmitCur;
    cmdTable["showServer"] = *cur;

    cur->init("showCmdTable", nullptr);
    cur->type = cur->TransmitCur;
    cmdTable["showCmdTable"] = *cur;
    
    cur->init("slaveof", nullptr);
    cur->type = cur->TransmitCur;
    cmdTable["slaveof"] = *cur;
}

void UserServer::serverRun() {
    epoll_event events[MAX_EVENT];
    Clients = new UserClient[MAX_EVENT];
    // 监听标准输入STD_IN
    struct sockaddr_in addr_stdin;
    Clients[0].initSTDIN(this);

    while(!flag_runDone) {
        int num = epoll_wait(fd_Epoll, events, MAX_EVENT, -1);
        //阻塞时被信号中断后返回错误
        if((num < 0) && (errno != EINTR)) {
            perror("epoll_wait()");
            flag_runDone = true;
            break;
        }

        for(int i = 0; i < num ; i++) {
            int fd_event = events[i].data.fd;
            if(fd_event == fd_server) {
                if(openDebug)
                    cout << "竟然会有人连接这个UserServer,真是不可思议!" << endl;
            } else if (fd_event == STDIN_FILENO) {
                if(Clients[fd_event].clientRead()) 
                    Clients[fd_event].STDINRunCommand();
            } else if(events[i].events & EPOLLIN) {
                if(Clients[fd_event].clientRead()) {
                    Clients[fd_event].runCommand();
                } else {
                    Clients[fd_event].closeClient();
                }
            } 
        }
    }
}

int UserServer::connectServer(UserClient* c) {
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
    Clients[sockfd_client].clientType = UserClient::Type::followerClient;
    time(&Clients[sockfd_client].timeLastChat);//不初始化的话,一下子就变成候选者了

    followerClients.emplace(Clients[sockfd_client].name, &Clients[sockfd_client]);   
    return sockfd_client;  
}

void UserServer::clientTypeChanged(string name, string whatType) {
    if(whatType == "leader") {
        if(leaderClient && leaderClient->name == name)
            cout << "该数据库服务器的身份已经是leader" << endl;
        else {
            auto it = followerClients.find(name);
            if(it == followerClients.end()) {
                cout << "不存在的数据库服务器" << endl;
            } else {
                if(leaderClient) {
                    // 原来是存在一个主服务器的,把它加入从服务器
                    followerClients[leaderClient->name] = leaderClient;
                }
                leaderClient = it->second;
                it->second->clientType = UserClient::leaderClient;
                followerClients.erase(name);
                cout << "已经更新leader数据库服务器" << endl;
            }
        }
    } else if(whatType == "cur") {
            auto it = followerClients.find(name);
            if(it == followerClients.end()) {
                cout << "不存在的数据库服务器" << endl;
            } else {
                curClient = it->second;
                cout << "已经更新cur数据库服务器" << endl;
            }
    }
}

void UserServer::showServer() {
    cout << "-------------------------------------" << endl;
    cout << "----------server message-------------" << endl;
    cout << "-------------------------------------" << endl;
    cout <<left<< setw(12) << "config_path:" << config_path << endl;
    cout <<left<< setw(12) << "fd_Epoll:" << fd_Epoll << endl;
    cout <<left<< setw(12) << "fd_server:" << fd_server << endl;
    cout <<left<< setw(12) << "port:" << port << endl;
    cout <<left<< setw(12) << "name:" << name << endl;
    cout << "-------------------------------------" << endl;
    cout << "-------------------------------------" << endl;
}

void UserServer::showClients() {
    cout << "-------------------------------------" << endl;
    if(leaderClient) {
        cout<< name + " : Leader Client"<< endl;
        cout << "-------------------------------------" << endl;
        leaderClient->showClient();
        cout << "-------------------------------------" << endl;
    } 
    if(curClient) {
        cout<< name + " : Cur Client"<< endl;
        cout << "-------------------------------------" << endl;
        curClient->showClient();
        cout << "-------------------------------------" << endl;
    } 

    cout << "-------------------------------------" << endl;
    cout<< name + " : Follower Clients"<< endl;
    cout << "-------------------------------------" << endl;
    for(auto it : followerClients)
        it.second->showClient();
}

void UserServer::showCmdTable() {
    int cnt = 1;
    for(auto it = cmdTable.begin(); it != cmdTable.end(); it++) {
        cout<< name + " UsefulCmds: "<< setw(3) << cnt++ << it->first << endl;
    }
}