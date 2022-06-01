#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>

#define BUF_SIZE 32
const int DEBUG = 1;
const int MAX_EVENT = 10;
void error_handling(char *message);
void add_event(int fd_epoll, int fd, int one_short, int opt);
void mod_event(int fd_epoll, int fd_socket, int ev);

int main(int argc, char *argv[])
{
	int fd_server, clnt_sock;
	char *message = (char*)malloc(BUF_SIZE);
	int str_len, i;
	
	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz;
	
	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	fd_server=socket(PF_INET, SOCK_STREAM, 0);   
	if(fd_server==-1)
		error_handling("socket() error");
	int opt = 1;
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(fd_server, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");
	
	if(listen(fd_server, 50)==-1)
		error_handling("listen() error");
	
    struct epoll_event events[MAX_EVENT];
    int fd_epoll = epoll_create(64);

        //添加监听文件
    add_event(fd_epoll, fd_server, 0, 0);

	int SIZE_BUF = 1640;
	int flag_readEnd = 0;
	char *buffer = (char*)malloc(SIZE_BUF);
	int len_read;
	int fd_client;
        //开始监听
    while(1) {
        if(DEBUG) {
            printf("Epoll listening...\n");
        }

        int num = epoll_wait(fd_epoll, events, MAX_EVENT, -1);
        //阻塞时被信号中断后返回错误
        if(num < 0) {
            error_handling("epoll_wait()");
        }if(DEBUG)
            printf("EPOLL监听到事件...\n");
        
        for(int i = 0; i < num ; i++) {
            int fd_event = events[i].data.fd;
            if(fd_event == fd_server) {
                //服务器监听套接字响应，有新的客户端连接
                if(DEBUG) {
                    printf("服务器响应,有客户端连接...\n");
                }

                struct sockaddr_in addr_client;
                socklen_t len_addr = sizeof(addr_client); 
                fd_client = accept(fd_server, (struct sockaddr*)&addr_client, &len_addr);
				add_event(fd_epoll, fd_client, 1, 0);
			} //后面监听到的都是客户端事件
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //对面发生异常断开或者错误监听
                close(fd_event);
            } else if(events[i].events & EPOLLIN) {
				// 小缓存多次读取试试,成功！
				len_read = read(fd_event, buffer, SIZE_BUF);
				printf("message from client:\n%s",buffer);
				if(len_read == 0) //一次读取到缓存区,没结束就先计算,然后输出
					flag_readEnd = 1;
				
				mod_event(fd_epoll, fd_event, EPOLLOUT);

            } else if(events[i].events & EPOLLOUT) {
				write(fd_event, buffer, len_read);
                if(DEBUG)
                    printf("write...\n");
				if(flag_readEnd) {
					flag_readEnd = 0;
					printf("模拟断开连接...\n");
					continue;
				}
				mod_event(fd_epoll, fd_event, EPOLLIN);
            }
        } 
    }

	close(fd_server);
	return 0;
}

void error_handling(char *message)
{
	perror(message);
	fputc('\n', stderr);
	exit(1);
}

void add_event(int fd_epoll, int fd, int one_short, int opt) {
    struct epoll_event event;
    event.data.fd = fd;
    //监听数据读取和连接断开
    event.events = EPOLLIN | EPOLLRDHUP ;
    if(opt == 0) {
        event.events = EPOLLIN  | EPOLLRDHUP ;
    }else if(opt == 1) {
        event.events = EPOLLOUT | EPOLLRDHUP ;
    }
    //oneshort:防止一个文件描述符被多个事件触发
    if(one_short) {
        event.events |= EPOLLONESHOT;
    }   

    epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &event);
    //设置文件描述符非阻塞,和ET,LT之间的关系？
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(fd,F_SETFL, flag, NULL);
}

void mod_event(int fd_epoll, int fd_socket, int ev) {
    struct epoll_event event;
    event.data.fd = fd_socket;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(fd_epoll, EPOLL_CTL_MOD ,fd_socket, &event);
}