#include "httpcon.h"

//利用sigaction设置信号和函数处理
void set_signal(int sig, void (*handler)(int)) {
    struct sigaction siga;
    bzero(&siga, sizeof(siga));
    siga.sa_handler = handler;
    //设定阻塞
    sigfillset(&siga.sa_mask);
    sigaction(sig,&siga,NULL);
}

void add_event(int fd_epoll, int fd, bool one_short) {
    epoll_event event;
    event.data.fd = fd;
    //监听数据读取和连接断开
    event.events = EPOLLIN | EPOLLHUP ;
    //oneshort:防止一个文件描述符被多个事件触发
    if(one_short) {
        event.events |= EPOLLONESHOT;
    }   

    epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞,和ET,LT之间的关系？
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd, flag, NULL);
}

void delete_event(int fd_epoll, int fd_socket) {
    epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_socket, NULL);
    close(fd_socket);
}

void mod_event(int fd_epoll, int fd_socket, int ev) {
    epoll_event event;
    event.data.fd = fd_socket;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(fd_epoll, EPOLL_CTL_MOD ,fd_socket, &event);
}

int HttpCon::m_epollfd = -1;
int HttpCon::m_user_cnt = 0;

void HttpCon::init(int sockfd,const sockaddr_in &addr) {
    m_socketfd = sockfd;
    m_addr = addr;

    int flag_set = 1;
    setsockopt(m_socketfd, SOL_SOCKET, SO_REUSEADDR, &flag_set, sizeof(flag_set));
    add_event(m_epollfd, sockfd, true);

    m_user_cnt ++;
}

void HttpCon::close() {
    if(m_socketfd != -1) {
        delete_event(m_epollfd, m_socketfd);
        m_socketfd = -1;
        m_user_cnt --;
    }

}

bool HttpCon::read() {
    printf("非阻塞一次性读出所有数据\n");
    return true;
}

bool HttpCon::write() {
    printf("非阻塞写入数据\n");
    return true;
}

void HttpCon::process() {
    //工作主要内容
    //读取HTTP报文
    char line[1024]={0};
    int len=get_line(m_socketfd,line,sizeof(line));
    if (len==0){
        mod_event(m_epollfd,m_socketfd,0);//客户端没有发送报文，修改事件重新监听，触发事件退出
    }
    else{
        while(true){
            char buffer[1024]={0};
            len=get_line(m_socketfd,buffer,sizeof(buffer));
            if (buffer[0]=='\n'){
                break;
            }
            else if (len==-1){
                break;
            }        
        }
    }
    if(strncasecmp("GET",line,3)==0){
        //处理Get请求
        http_request(line,m_socketfd,len);
        delete_event(m_epollfd,m_socketfd);
        //这里是修改还是关闭待讨论
        //mod_event(m_epollfd,m_socketfd,0);
    }
    else if(strncasecmp("POST", line, 4) == 0){ // 这里还要固定文件名 正则表达式拿下来
        // 实体行里面没有换行符号 读一次就行
        // 这里不用设置长度了 使用了定时器的话
        http_request(line,m_socketfd,len);
        delete_event(m_epollfd,m_socketfd);
    
    }
    else{
        http_request(line,m_socketfd,len);
        delete_event(m_epollfd,m_socketfd);
    }
    printf("工作执行中......\n");
}
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}
void send_error(int cfd, int status, char *title, char *text)
{
	char buf[4096] = {0};

	sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
	sprintf(buf+strlen(buf), "Content-Type:%s\r\n", "text/html");
	sprintf(buf+strlen(buf), "Content-Length:%d\r\n", -1);// 这里长度设置为-1，也可以写在下面 用strlen求出buf的长度
	sprintf(buf+strlen(buf), "Connection: close\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);
    send_file(cfd,"./static/404.html");
    /*
	memset(buf, 0, sizeof(buf));

	sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf+strlen(buf), "%s\n", text);
	sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
	send(cfd, buf, strlen(buf), 0);
	*/
	return ;
}
// 发送目录内容
void send_dir(int cfd, const char* dirname)
{
    int i, ret;

    // 拼一个html页面<table></table>
    char buf[4094] = {0};

    sprintf(buf, "<html><head><title>目录名: %s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>当前目录: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};
    
    // 目录项二级指针
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);
    
    // 遍历
    for(i = 0; i < num; ++i) {
    
        char* name = ptr[i]->d_name;

        // 拼接文件的完整路径
        sprintf(path, "%s/%s", dirname, name);
        //printf("path = %s ===================\n", path);
        struct stat st;
        stat(path, &st);

		// 编码生成 %E5 %A7 之类的东西
        encode_str(enstr, sizeof(enstr), name);
        
        // 如果是文件
        if(S_ISREG(st.st_mode)) {      
            // 这里如果不编码发过去，对面会打不开本地的中文文档和目录 
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        } else if(S_ISDIR(st.st_mode)) {		// 如果是目录       
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
        // 字符串拼接
    }

    sprintf(buf+strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);

    //printf("dir message send OK!!!!\n");
#if 0
    // 打开目录
    DIR* dir = opendir(dirname);
    if(dir == NULL)
    {
        perror("opendir error");
        exit(1);
    }

    // 读目录
    struct dirent* ptr = NULL;
    while( (ptr = readdir(dir)) != NULL )
    {
        char* name = ptr->d_name;
    }
    closedir(dir);
#endif
}

// 发送响应头
void send_respond_head(int cfd, int no, const char* desp, const char* type, long len)
{
    char buf[1024] = {0};
    // 状态行
    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desp);
    send(cfd, buf, strlen(buf), 0);
    // 消息报头
    sprintf(buf, "Content-Type:%s\r\n", type);
    sprintf(buf+strlen(buf), "Content-Length:%ld\r\n", len);
    send(cfd, buf, strlen(buf), 0);
    // 空行
    send(cfd, "\r\n", 2, 0);
}

// 发送文件
void send_file(int cfd, const char* filename)
{
    // 打开文件
    int fd = open(filename, O_RDONLY);
    if(fd == -1) {
        send_error(cfd, 404, "Not Found", "NO such file or direntry");
        exit(1);
    }

    // 循环读文件
    char buf[4096] = {0};
    int len = 0, ret = 0;
    // 设置成了非阻塞
    while( (len = read(fd, buf, sizeof(buf))) > 0 ) {   
        // 发送读出的数据
        ret = send(cfd, buf, len, 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
    }
    if(len == -1)  {  
        perror("read file error");
        exit(1);
    }

    close(fd);
}
//发送数据
void send_data(int cfd, int status, char *title, char *text){
    char buf2[1024] = {0};
    sprintf(buf2, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf2+strlen(buf2), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf2+strlen(buf2), "%s\n", text);
	sprintf(buf2+strlen(buf2), "<hr>\n</body>\n</html>\n");
    send(cfd, buf2, strlen(buf2), 0);
	return;
}
//编码字节流
void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {    
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {      
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

void decode_str(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}const char *get_file_type(char *name)
{
    char* dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL
    dot = strrchr(name, '.');   
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
// 解析http请求消息的每一行内容
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n')) { 
        // 这个地方是阻塞接收，可以定个时   
        n = recv(sock, &c, 1, 0);
        if (n > 0) {        
            if (c == '\r') {            
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n')) {               
                    recv(sock, &c, 1, 0);
                } else {                            
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        } else {       
            c = '\n';
        }
    }
    buf[i] = '\0';

    return i;
}
bool get_name_id(const char *str, int len, char *name, char *id,int *post_len){
    int spite = 0;
    while(spite < len && str[spite] != '&')
        ++spite;
    if(spite == len)
        return false;

    // 获取name
    int equal1 = 0;
    while(equal1 < spite && str[equal1] != '=')
        ++equal1;
    if(!(equal1 == 4 && str[0] == 'N' && str[1] == 'a' && str[2] == 'm' && str[3] == 'e'))
        return false;
    int index1 = 0;
    for(int i = equal1 + 1; i < spite; ++i){
        name[index1++] = str[i];
    }
    name[index1] = '\0';

    // 获取id
    int equal2 = spite + 1;
    while(equal2 < len && str[equal2] != '=')
        ++equal2;
    if(!(equal2 - spite == 3 && str[spite + 1] == 'I' && str[spite + 2] == 'D'))
        return false;
    int index2 = 0;
    for(int i = equal2 + 1; i < len; ++i){
        id[index2++] = str[i];
    }
    id[index2] = '\0'; 
    *post_len=index1+index2;
    return true;
}
// http请求处理
void http_request(const char* request, int cfd,int len=0)
{
    // 拆分http请求行
    char method[12], path[1024], protocol[12];
    int *error_len;
    sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);

    // 转码 将不能识别的中文乱码 -> 中文
    // 解码 %23 %34 %5f
    decode_str(path, path);
        
    char* file = path + 1; // 去掉path中的/ 获取访问文件名
    //默认是当前路径 也就是把第一个/去掉
    if(method=="POST"){
        char name[50], id[50];
        if(strcmp(path, "/api/echo") != 0){ // 路径不对 或者键值对不对
                send_error(cfd, 404, "Not Found", "Not Found"); 
                return;
            }
        else if(!get_name_id(request, len, name, id,error_len)){
            struct stat st;
            int ret = stat("./data/error.txt", &st);
            send_respond_head(cfd,404,"Not Found","text/plain",st.st_size);
            send_file(cfd,"./data/error.txt");
            return;
        }
            char dest[1024] = {'\0'};
            strcat(dest, "Your Name: ");
            strcat(dest, name);
            strcat(dest, "\nID: ");
            strcat(dest, id);
            send_respond_head(cfd,200,"OK","application/x-www-form-urlencoded",*error_len);
            send_data(cfd, 200, "OK", dest);
    }
    else if(method=="GET"){
        // 如果没有指定访问的资源, 默认显示资源目录中的内容
        if(strcmp(path, "/") == 0) {    
            // file的值, 资源目录的当前位置
            file = "./static/index.html";
        }

        // 获取文件属性
        struct stat st;
        int ret = stat(file, &st);
        if(ret == -1) { 
            send_error(cfd, 404, "Not Found", "NO such file or direntry");     
            return;
        }

        // 判断是目录还是文件
        if(S_ISDIR(st.st_mode)) {  		// 目录 
            // 发送头信息
            send_respond_head(cfd, 200, "OK", get_file_type(".html"), -1);
            // 发送目录信息
            send_dir(cfd, file);
        } else if(S_ISREG(st.st_mode)) { // 文件        
            // 发送消息报头
            send_respond_head(cfd, 200, "OK", get_file_type(file), st.st_size);
            // 发送文件内容
            send_file(cfd, file);
        }
    }
    else{
        char dest[1024] = {'\0'};
        strcat(dest, "Does not implement this method: ");
        strcat(dest, method);
        send_respond_head(cfd,501,"Not Implemented",get_file_type(".html"),33);
        send_file(cfd,"./static/501.html");
    }
    
}