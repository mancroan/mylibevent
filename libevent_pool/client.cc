#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <event.h>
#include <time.h>   //用到clock()函数

using namespace std;

#define BUF_SIZE 1024

/**
 * 连接到server端，如果成功，返回fd，如果失败返回-1
 */
int connectServer(char* ip, int port){
    int fd = socket( AF_INET, SOCK_STREAM, 0 );
    if(-1 == fd){
        cout<<"Error, connectServer() quit"<<endl;
        return -1;
    }
    struct sockaddr_in remote_addr; //服务器端网络地址结构体
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family=AF_INET; //设置为IP通信
    remote_addr.sin_addr.s_addr=inet_addr(ip);//服务器IP地址
    remote_addr.sin_port=htons(port); //服务器端口号
    int con_result = connect(fd, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr));
    if(con_result < 0){
        cout<<"Connect Error!"<<endl;
        close(fd);
        return -1;
    }
    return fd;
}

int main() {

	int begintime = time(0);
	for(int i=0; i<100000; i++)
	{
	    int socket_fd = connectServer((char*)"127.0.0.1", 8888);
	    char *buffer = (char*)"hello world!";
	    int write_num = write(socket_fd, buffer, strlen(buffer));

	    char recvbuf[1024] = {0};

	    read(socket_fd, recvbuf, BUF_SIZE);
	    cout<<"server:"<<recvbuf<<" fd="<<socket_fd<<endl;
	    close(socket_fd);
    }
    int endtime = time(0);

	printf("\n\nRunning Time：%ds\n", endtime-begintime);
    return 0;
}
