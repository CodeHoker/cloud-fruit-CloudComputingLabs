#include "all.hpp"
#include "fun.hpp"
using namespace std;

 
int fd_client = 0;//输入输出线程需要的文件描述符
int main(int argc, char *argv[]) {
 
    if(argc >= 2) 
        readConfig(argv[1]);
    else
        readConfig(path_config);
    cout << config["serverIP"] << endl;
    cout << config["serverPort"] << endl;
    init();
	
    // socket() 
	int sockfd_client;
	sockfd_client = socket(PF_INET, SOCK_STREAM, 0); 
    fd_client = sockfd_client; 
    if(sockfd_client == -1)
        error_handling("socket()");

    //connect()
	struct sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr)); 
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(config["serverIP"].c_str());
	serv_adr.sin_port=htons(atoi(config["serverPort"].c_str()));
	 
	if(connect(sockfd_client, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("connect()");

    pthread_t thread_read  ;
    pthread_t thread_write ;
    pthread_create(&thread_read, 0, read_routine, NULL);
    pthread_create(&thread_write, 0, write_routine, NULL);
    pthread_detach(thread_read);
    pthread_detach(thread_write);
    pthread_exit(0);

	return 0;
}
