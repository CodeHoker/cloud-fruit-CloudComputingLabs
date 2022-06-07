unordered_map<string,string> store;//先用一个暂时的全局变量进行存储 
string setCommand(vector<string> argv) {
    if(argv[0] != "set" && argv[0] != "SET")
        return "-ERROR\r\n";
    string str_t(argv[2]);
    int pos = 3;
    while(pos < argv.size()) {
        str_t.append(" "+argv[pos++]);
    }
    store[argv[1]] = str_t;
    return "+OK\r\n";
}

string getCommand(vector<string> argv) {
    if(argv[0] != "get" && argv[0] != "GET")
        return "-ERROR\r\n";
    if(store.find(argv[1]) == store.end())
        return "*1\r\n$3\r\nnil\r\n";

    // 这个函数需要有两种,一种是RESP编码类型,一种是可视化类型
    // retStr = store[argv[1]]+"\r\n";//可视化返回
    // 下面是对返回结果进行编码
    vector<string> ret;
    ret.push_back("*");
    stringstream ss(store[argv[1]]);
    string str;
    while(ss >> str) {
        ret.emplace_back("$");
        ret.emplace_back(str);
    }
    return encodeResp(ret, 0);
}

string delCommand(vector<string> argv) {
    if(argv[0] != "del" && argv[0] != "DEL")
        return "-ERROR\r\n";
    int cnt = 0;
    for(int i = 1; i < argv.size(); i++) {
        auto it = store.find(argv[i]);
        if(it == store.end())
            continue;
        store.erase(it);
        cnt++;
    }

    return ":" + to_string(cnt) + "\r\n";
}