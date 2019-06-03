#include<sys/types.h>  
#include<errno.h>  
#include<stdio.h>  
#include<string.h>  
#include<stdlib.h> 

#ifdef _MSC_VER
#include <WinSock2.h>
#include<ws2tcpip.h>
#else
#include <unistd.h>
#include <netinet/in.h>  
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif
  
#include<event.h>  
#include<event2/bufferevent.h>  
#include<event2/buffer.h>  
#include<event2/util.h>  
  
  
  
  
int tcp_connect_server(const char* server_ip, int port);  
  
  
void cmd_msg_cb(int fd, short events, void* arg);  
void server_msg_cb(struct bufferevent* bev, void* arg);  
void event_cb(struct bufferevent *bev, short event, void *arg);  
  
int main(int argc, char** argv)  
{  

#ifdef _MSC_VER
	WSADATA  Ws;
	//Init Windows Socket
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0)
	{
		return -1;
	}
#endif
  
    //�������������Ƿ������˵�IP��ַ���˿ں�  
    int sockfd = tcp_connect_server("127.0.0.1", 9999);  
    if( sockfd == -1)  
    {  
        perror("tcp_connect error ");  
        return -1;  
    }  
  
    printf("connect to server successful\n");  
  
    struct event_base* base = event_base_new();  
  
    struct bufferevent* bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);  
  
    //�����ն������¼�  
    struct event* ev_cmd = event_new(base, STDIN_FILENO,  
                                      EV_READ | EV_PERSIST, cmd_msg_cb,  
                                      (void*)bev);  
    event_add(ev_cmd, NULL);  
  
    //��socket�ر�ʱ���õ��ص�����  
    bufferevent_setcb(bev, server_msg_cb, NULL, event_cb, (void*)ev_cmd);  
    bufferevent_enable(bev, EV_READ | EV_PERSIST);  
  
  
    event_base_dispatch(base);  
  
    printf("finished \n");  
    return 0;  
}  
  
  
  
  
  
  
void cmd_msg_cb(int fd, short events, void* arg)  
{  
    char msg[1024];  
  
    int ret = read(fd, msg, sizeof(msg));  
    if( ret < 0 )  
    {  
        perror("read fail ");  
        exit(1);  
    }  
  
    struct bufferevent* bev = (struct bufferevent*)arg;  
  
    //���ն˵���Ϣ���͸���������  
    bufferevent_write(bev, msg, ret);  
}  
  
  
void server_msg_cb(struct bufferevent* bev, void* arg)  
{  
    char msg[1024];  
  
    size_t len = bufferevent_read(bev, msg, sizeof(msg));  
    msg[len] = '\0';  
  
    printf("recv %s from server\n", msg);  
}  
  
  
void event_cb(struct bufferevent *bev, short event, void *arg)  
{  
  
    if (event & BEV_EVENT_EOF)  
        printf("connection closed\n");  
    else if (event & BEV_EVENT_ERROR)  
        printf("some other error\n");  
  
    //�⽫�Զ�close�׽��ֺ�free��д������  
    bufferevent_free(bev);  
  
    struct event *ev = (struct event*)arg;  
    //��Ϊsocket�Ѿ�û�У��������eventҲû�д��ڵı�Ҫ��  
    event_free(ev);  
}  
  
  
typedef struct sockaddr SA;  
int tcp_connect_server(const char* server_ip, int port)  
{  
    int sockfd, status, save_errno;  
    struct sockaddr_in server_addr;  
  
    memset(&server_addr, 0, sizeof(server_addr) );  
  
    server_addr.sin_family = AF_INET;  
    server_addr.sin_port = htons(port);  
    status = inet_aton(server_ip, &server_addr.sin_addr);  
  
    if( status == 0 ) //the server_ip is not valid value  
    {  
        errno = EINVAL;  
        return -1;  
    }  
  
    sockfd = socket(PF_INET, SOCK_STREAM, 0);  
    if( sockfd == -1 )  
        return sockfd;  
  
  
    status = ::connect(sockfd, (SA*)&server_addr, sizeof(server_addr) );  
  
    if( status == -1 )  
    {  
        save_errno = errno;  
        ::close(sockfd);  
        errno = save_errno; //the close may be error  
        return -1;  
    }  
  
    evutil_make_socket_nonblocking(sockfd);  
  
    return sockfd;  
}