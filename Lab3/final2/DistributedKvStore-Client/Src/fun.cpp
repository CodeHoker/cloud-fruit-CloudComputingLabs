#include "all.hpp"
#include "fun.hpp"

using namespace std;


void error_handling(const char *message) {
	perror(message);
	fputc('\n', stderr);
	exit(1);
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
		printf("Message from server:\n%s\n", buf);
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
		write(fd_client, cmd, strlen(cmd));
	}
    return 0;
}

void set_async() {
    fcntl(0, F_SETOWN, getpid());

    int flags = fcntl(0, F_GETFL);
    flags |= O_ASYNC;    
    fcntl(0, F_SETFL, flags);

    signal(SIGIO, async_input);
}


void async_input(int sig) {
    if(sig == SIGIO) {
        vector<string> ret;
        ret.emplace_back("*");
        string str;
        while(cin >> str) {
            ret.emplace_back("$");
            ret.emplace_back(str);
            // 以换行结束
            if(cin.get() == '\n')
                break;
        }
        cout << "客户端输入1" << endl;
        str = encodeResp(ret, 0);
        cout << "客户端输入2" << endl;
        cout << str << endl;
        Queue_Cmd.emplace_back(str.c_str());
        sem_post(&sem_cmd);
        return;
    }
}


list<char*> Queue_Cmd;
pthread_mutex_t Cmd_Queue;
sem_t sem_cmd;

void init() {
    pthread_mutex_init(&Cmd_Queue, 0);
    sem_init(&sem_cmd, 0, 0);
    set_async();
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

char* takeCmd_FromQueue() {

    char *filename;

    sem_wait(&sem_cmd);
    pthread_mutex_lock(&Cmd_Queue);

    filename = Queue_Cmd.front();
    Queue_Cmd.pop_back();
    pthread_mutex_unlock(&Cmd_Queue);
    return filename;
}

const char* path_config = "../config/config.txt";
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