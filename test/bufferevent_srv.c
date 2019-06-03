
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <event.h>
#include <event2/bufferevent.h>

#ifdef _MSC_VER
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

struct msg
{
	int fd;
	char buf[1024];
};


void accept_cb(int fd, short events, void* arg);
void socket_read_cb(struct bufferevent* bev, void* arg);
void socket_write_cb(struct bufferevent* bev, void* arg);
void event_cb(struct bufferevent *bev, short event, void *arg);
int tcp_server_init(const char* ip, int port, int listen_num);

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

	int listenfd = tcp_server_init("127.0.0.1", 9999, 1024);
	if( listenfd == -1 )
	{
		perror(" tcp_server_init error ");
		return -1;
	}

	struct event_base* base = event_base_new();
	struct event* ev_listen = event_new(base, listenfd, EV_READ | EV_PERSIST, (event_callback_fn)accept_cb, base);
	event_add(ev_listen, NULL);

	printf("===========================================\n");
	printf("              gameServer OK\n");
	printf("===========================================\n");

	event_base_dispatch(base);
	event_base_free(base);

#ifdef _MSC_VER
	WSACleanup();
#endif


	return 0;
}

void accept_cb(int fd, short events, void* arg)
{
	static evutil_socket_t clientfd;

	struct sockaddr_in client;
	socklen_t len = sizeof(client);

	clientfd = accept(fd, (struct sockaddr*)&client, &len );
	evutil_make_socket_nonblocking(clientfd);

	printf("accept a client %d\n", clientfd);

	struct event_base *base = (struct event_base *)arg;

	struct bufferevent* bev = bufferevent_socket_new(base, clientfd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, socket_read_cb, socket_write_cb, event_cb, &clientfd);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
}

void socket_read_cb(struct bufferevent* bev, void* arg)
{
	int clientfd = *(int*)arg;
	char buf[1024] = {0};
	size_t len = bufferevent_read(bev, buf, sizeof(buf));
	printf("recv the client msg: %s\n", buf);

	char *hello = "i am server";
	send(clientfd, hello, strlen(hello), 0);
	//bufferevent_write(bev, hello, strlen(hello));
}

void socket_write_cb(struct bufferevent* bev, void* arg)
{
	printf("@@@@@@@@@@@@@@@@@@");
}


void event_cb(struct bufferevent *bev, short eventid, void *arg)
{
	if (eventid & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	}
	else if (eventid & BEV_EVENT_ERROR) {
		printf("Some other error.\n");
	}
	else if (eventid & BEV_EVENT_CONNECTED) {
		printf("Client has successfully connected.\n");
		return;
	}
	bufferevent_free(bev);
}

typedef struct sockaddr SA;
int tcp_server_init(const char* ip, int port, int listen_num)
{
	int errno_save;
	evutil_socket_t listenfd;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if( listenfd == -1 )
		return -1;


	struct sockaddr_in local_addr; //服务器端网络地址结构体
	memset(&local_addr, 0, sizeof(local_addr)); //数据初始化--清零
	local_addr.sin_family = AF_INET; //设置为IP通信
	local_addr.sin_addr.s_addr = inet_addr(ip);//服务器IP地址
	local_addr.sin_port = htons(port); //服务器端口号

	//允许多次绑定同一个地址。要用在socket和bind之间
	evutil_make_listen_socket_reuseable(listenfd);


	if(bind(listenfd, (SA*)&local_addr, sizeof(local_addr)) < 0 )
		goto error;

	if(listen(listenfd, listen_num) < 0)
		goto error;


	evutil_make_socket_nonblocking(listenfd);

	return listenfd;

error:
	errno_save = errno;
	evutil_closesocket(listenfd);
	errno = errno_save;

	return -1;
}