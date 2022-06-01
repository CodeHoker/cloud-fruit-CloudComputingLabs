#include "all.hpp"
using namespace std;

void handler_error(const char *message) {
    perror(message);
    exit(-1);
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

vector<string> decodeResp(const char* buffer, string& str) {
    // cout << "即将解析的数据\n" << buffer;
    str.assign(buffer);
    string str_t;
    vector<string> ret;
    // *2\r\n$5\r\nCloud\r\n$9\r\nComputing\r\n
    switch(buffer[0]) {
        case '-':
        case '+':
        case ':':
            ret.emplace_back(str.substr(1, str.length()-3));
        case '*':
            bool flag_isStr = false;//记录是否读取一个字符串
            bool flag_isdollar = false;//记录是否读取到一个'$'
            for(int i = 1;i < str.length(); i++) {
                if(str[i] == '\r' && flag_isStr) {
                    flag_isdollar = false;
                    flag_isStr = false;
                    ret.emplace_back(str_t);
                } else if(str[i] == '\n' && flag_isdollar) {
                    flag_isStr = true;
                    str_t.assign("");
                } else if(str[i] == '$') {
                    flag_isdollar = true;
                } else if(flag_isStr)
                    str_t.push_back(str[i]);
            }

        break;
    }
    return ret;
}

vector<string> input2Argc(istream& in) {
    vector<string> ret;
    ret.emplace_back("*");
    string str;
    while(in >> str) {
        ret.emplace_back("$");
        ret.emplace_back(str);
        // 以换行结束
        if(in.get() == '\n')
            break;
    }
    return ret;
}

void setAlarm(long sec) {
    struct itimerval clock;
    //毫秒不用的时候需要设置为0
    clock.it_value.tv_sec  = sec;
    clock.it_value.tv_usec = 0;
    clock.it_interval.tv_sec  = sec;
    clock.it_interval.tv_usec =0;

    setitimer(ITIMER_REAL, &clock, NULL);
}