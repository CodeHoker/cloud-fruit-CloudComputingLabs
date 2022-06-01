#ifndef NEWFUN_HPP
#define NEWFUN_HPP
#include "all.hpp"
using namespace std;

// 错误处理
void error_handling(const char *message);
extern int fd_client;
const int BUF_SIZE = 1024;
// 配置文件解析
extern unordered_map<string,string> config;
extern const char* path_config;
bool readConfig(const char* filepath);

// 初始化
void init();
extern list<char*> Queue_Cmd;    //用来存放指令
extern pthread_mutex_t cmd_Queue;//用来控制指令存取
extern sem_t sem_cmd;
// 异步设置
void set_async();
// C++风格输入
void async_input(int sig);
// resp编码
string encodeResp(const vector<string>& resp, size_t pos);
// 读线程
extern int fd_client;
extern int flag_readDone;
void* read_routine(void*);
// 写线程
extern int flag_writeDone;
void* write_routine(void*);
// 从队列中拿去命令
char* takeCmd_FromQueue();

#endif