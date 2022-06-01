#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "all.hpp"
#include "semaphore.h"
using namespace std;

extern int fd_client;
extern const int BUF_SIZE;

void error_handling(const char *message) {
	perror(message);
	fputc('\n', stderr);
	exit(1);
}

const char* path_config = "./config/config.txt";
unordered_map<string,string> config;//保存配置文件
bool readConfig(const char* filepath) {

    fstream file_config;
    file_config.open(filepath);//打开文件	
    if(!file_config.is_open()) {
        error_handling("readConfig()");
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
    return true;
}

list<char*> Queue_Cmd;    
pthread_mutex_t cmd_Queue;
sem_t sem_cmd;
void init() {
    pthread_mutex_init(&cmd_Queue, 0);
    sem_init(&sem_cmd, 0, 0);
    set_async();
}

void set_async() {
    fcntl(0, F_SETOWN, getpid());

    int flags = fcntl(0, F_GETFL);
    flags |= O_ASYNC;    
    fcntl(0, F_SETFL, flags);

    signal(SIGIO, async_input);
}

void async_input(int sig) {
    // cout << "hello world " << endl;,至此都是正常的
    if(sig == SIGIO) {
        vector<string> ret;
        ret.emplace_back("*");
        string str;
        while(getline(cin,str)) {
            stringstream ss(str);
            string s_t;
            while(ss>>s_t) {
                ret.emplace_back("$");
                ret.emplace_back(s_t);
            }
            break;
            // 以换行结束
            // 问题：说出来你可能不信
            // 当一行输入被读取完的时候，cin.get会透支即将输入的
            // if(cin.get() == '\n')
            //     break;
        }
        str = encodeResp(ret, 0);

        // 将指令加入链表,方便线程读取线程进行拿取
        // pthread_mutex_lock(&cmd_Queue);
        // 错误发生在这里,直接使用c_str好像并没有指针,申请内存要+1
        char *s = (char*)malloc(strlen(str.c_str()) +1);
        strcpy(s, str.c_str());
        Queue_Cmd.emplace_back(s);
        // pthread_mutex_unlock(&cmd_Queue);
        sem_post(&sem_cmd);
        return;
    }
}

string encodeResp(const vector<string>& resp, size_t pos) {
    string ret;
    ret.assign(resp[pos]);
    switch (resp[pos][0])
    {
    case '+': ;
    case '-': ; 
    case ':': ;
        ret.append(resp[pos+1]);
        ret.append("\r\n");
        break;
    case '$':
        ret.append(to_string(resp[pos+1].size()));
        ret.append("\r\n");
        ret.append(resp[pos+1]);
        ret.append("\r\n");
        break;
    case '*':
        ret.append(to_string(resp.size()/2));
        ret.append("\r\n");
        pos++;
        while(pos <= resp.size()-2) {
            ret.append(encodeResp(resp, pos));
            pos += 2;
        };
        break;
    }
    return ret;
}


int flag_readDone = 0;
void* read_routine(void*)
{
    char *buf = (char*)malloc(BUF_SIZE);
	while(!flag_readDone)
	{
		int str_len = read(fd_client, buf, BUF_SIZE);
		if(str_len == 0) {
            flag_readDone = 1;
            close(fd_client);
			return 0;
        }else if(str_len == -1 && errno == EINTR) 
            continue;

		buf[str_len] = '\0';
		printf("Message from server:\n%s", buf);
	}
    return 0;
}

int flag_writeDone = 0;
void* write_routine(void*)
{      
	while(!flag_writeDone)
	{
        char *cmd = takeCmd_FromQueue();
		if(!strcmp(cmd,"q") || !strcmp(cmd,"Q")) {	
			shutdown(fd_client, SHUT_WR);
            flag_writeDone = 1;
			return 0;
		}
        // cout << "take a cmd from queue\n" << cmd;
		write(fd_client, cmd, strlen(cmd));
	}
    return 0;
}

char* takeCmd_FromQueue() {

    char *filename;

    sem_wait(&sem_cmd);
    // pthread_mutex_lock(&cmd_Queue);

    filename = Queue_Cmd.back();
    Queue_Cmd.pop_back();
    // pthread_mutex_unlock(&cmd_Queue);
    return filename;
}