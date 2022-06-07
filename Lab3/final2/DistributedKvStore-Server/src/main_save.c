#include "all.hpp"
using namespace std;


int main_save() {
    // 客户端对象
    RedisServer server;
    server.init();
    // 读取配置文件
    readConfig(path_config);
    //处理信号,防止因为客户端的突然终止而导致服务器崩溃
    set_signal(SIGPIPE, SIG_IGN);

    //初始化线程池
        // Redis是单线程的,所以不需要线程池
        // 应该是任务分发和任务处理的结构

    //网络编程代码
    int fd_server = my_serverSocket();

    //epoll监听
        //epoll对象
    epoll_event events[MAX_EVENT];
    int fd_epoll = epoll_create(64);
        //添加监听文件
    add_event(fd_epoll, fd_server, false, EP_IN);

    vector<string> cmd;
    char* buffer_Read = (char*)malloc(1024);
    string str;
    while(true) {
        cout <<"waiting..." << endl;
        int num = epoll_wait(fd_epoll, events, MAX_EVENT, -1);
        //阻塞时被信号中断后返回错误
        if((num < 0) && (errno != EINTR)) {
            handler_error("epoll_wait()");
        }if(openDebug)
            printf("something is heppening...\n");

        for(int i = 0; i < num ; i++) {
            int fd_event = events[i].data.fd;
            int fd_client;
            if(fd_event == fd_server) {
                //服务器监听套接字响应，有新的客户端连接
                if(openDebug) {
                    printf("client connecting...\n");
                }
                struct sockaddr_in addr_client;
                socklen_t len_addr = sizeof(addr_client); 
                fd_client = accept(fd_server, (struct sockaddr*)&addr_client, &len_addr);
                add_event(fd_epoll, fd_client, true, EP_IN);
            }else if(events[i].events & EPOLLIN){
                int len_Read = read(fd_client, buffer_Read, 1024);
                buffer_Read[len_Read] = '\0';
                if(len_Read == 0 || len_Read == -1) {
                    close(fd_client);
                    cout << "disconnected..." << endl;
                }
                cmd = decodeResp(buffer_Read,str);
                // 简单任务分发器,指令直接返回的string,是RESP编码格式
                if(cmd[0] == "set" || cmd[0] == "SET")
                    str = setCommand(cmd);
                else if(cmd[0] == "get" || cmd[0] == "GET")
                    str = getCommand(cmd);
                else if(cmd[0] == "del" || cmd[0] == "DEL")
                    str = delCommand(cmd);
                else {
                    cout << "There is no such instruction" << endl;
                    continue;
                }
                cout << str;
                // 先填充缓冲区,写,然后设置EPOLLOUT
                strcpy(buffer_Read, str.c_str());
                mod_event(fd_epoll, fd_event, EPOLLOUT);
            }else if(events[i].events & EPOLLOUT) {
                cout << "writing..." << endl;
                // 写入失败的原因是,保存的数据str在循环中被清空了
                write(fd_client, buffer_Read, strlen(buffer_Read));
                mod_event(fd_epoll, fd_event, EPOLLIN);
            }
        }
    }
    return 0;
}

