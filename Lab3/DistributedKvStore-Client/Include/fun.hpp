#ifndef _FUN_HPP_
#define _FUN_HPP_
#include "all.hpp"
using namespace std;

// main

    // 输入输出线程
const int DEBUG = true;
extern int flag_readDone;
extern int flag_writeDone;
void* read_routine(void*);
void* write_routine(void*);

    // 异步输入
const int SIZE_FILE_NAME = 32;
extern list<char *> Queue_Cmd;
extern sem_t sem_cmd;
extern pthread_mutex_t Cmd_Queue;
void set_async();
void async_input(int sig);
char* wait_Intput(FILE* fileIn);
char* takeCmd_FromQueue();
void read_File(const char *fileName, int sock_client);
void error_handling(const char *message);
void init();

    // 配置文件读取
extern const char* path_config;
bool readConfig(const char* filepath);

    // RESP序列相关
    // resp编码
string encodeResp(const vector<string>& resp, size_t pos);

#endif