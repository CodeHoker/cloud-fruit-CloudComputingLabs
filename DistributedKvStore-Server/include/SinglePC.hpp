#ifndef SINGLEPC_HPP
#define SINGLEPC_HPP
#include "all.hpp"
using namespace std;

// --------------------------------
// 一些基础常量
// extern const char* path_config;
// extern unordered_map<string,string> config;
extern unordered_map<string,string> store;

// 错误处理
void handler_error(const char *message);
// 配置文件解析函数
// bool readConfig(const char* filepath);
// RESP序列
    // RESP编码
string encodeResp(const vector<string>& resp, size_t pos);
    // RESP解码,解码成为参数列表,str好像还没什么用,以后弄成可视化形式
vector<string> decodeResp(const char* buffer, string& str);
    // 将输入转为RESP序列编码前的数组
vector<string> input2Argc(istream& in); 
    // 定时器设置
void setAlarm(long sec); 
    


#endif