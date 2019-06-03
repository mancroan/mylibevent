//包含所需要的头文件
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"
#include "event2/buffer.h"

//监听回调函数
void listener_cb(evconnlistener *listener, evutil_socket_t fd,
struct sockaddr *sock, int socklen, void *arg);  

//从Socket接收消息的回调函数
void socket_read_cb(bufferevent *bev, void *arg);

//从Socket事件的回调函数
void socket_event_cb(bufferevent *bev, short events, void *arg);  

int main()
{  
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);
	//这里必须初始化网络，不然会创建Socket失败
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(2000);

	/*
	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(struct sockaddr_in6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = htons(2000);
	*/

	//告诉libEvent使用Windows线程
	//这句是必须的，不然会导致event_base_dispatch时一直处于Sleep状态，无法正常工作
	evthread_use_windows_threads();

	struct event_config* cfg = event_config_new();
	event_config_set_flag(cfg,EVENT_BASE_FLAG_STARTUP_IOCP);
	//根据CPU实际数量配置libEvent的CPU数
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	event_config_set_num_cpus_hint(cfg,si.dwNumberOfProcessors);

	event_base *base;
	base = event_base_new_with_config(cfg); 
	event_config_free(cfg);

	// 绑定并监听IPV4端口
	evconnlistener *listener = evconnlistener_new_bind(base, listener_cb, base,  
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,  
		10, (struct sockaddr*)&sin,  
		sizeof(sin));

	/*
	// 绑定并监听IPV6端口
	evconnlistener *listener6 = evconnlistener_new_bind(base, listener_cb, base,  
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,  
		10, (struct sockaddr*)&sin6,  
		sizeof(sin6));
	*/

	//事件分发处理
	event_base_dispatch(base);  

	evconnlistener_free(listener);
	//evconnlistener_free(listener6);
	event_base_free(base);
	WSACleanup();

	return 0;  
}

//一个新客户端连接上服务器了  
//当此函数被调用时，libevent已经帮我们accept了这个客户端。该客户端的
//文件描述符为fd
void listener_cb(evconnlistener *listener, evutil_socket_t fd,  
struct sockaddr *sock, int socklen, void *arg)  
{  
	char Buffer[256];
	sockaddr_in* addr = (sockaddr_in*)sock;
	evutil_inet_ntop(addr->sin_family,&addr->sin_addr,Buffer,sizeof(Buffer));
	printf("accept a client %d,IP:%s\n", fd,Buffer);

	event_base *base = (event_base*)arg;  

	//为这个客户端分配一个bufferevent  
	bufferevent *bev =  bufferevent_socket_new(base, fd,  
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);  

	bufferevent_setcb(bev, socket_read_cb, NULL, socket_event_cb, NULL);  
	bufferevent_enable(bev, EV_READ | EV_PERSIST);  
}

void socket_read_cb(bufferevent *bev, void *arg)  
{  
	char msg[4096];

	size_t len;
	// 这里一行一行的读取
	char* p = evbuffer_readln(bufferevent_get_input(bev),&len,EVBUFFER_EOL_ANY);
	if(p)
	{
		// 如果输入exit或者quit则退出程序
		// 可以使用event_base_loopexit或者event_base_loopbreak
		// 它们的区别是前者会把事件处理完才退出，后者是立即退出
		if(!strcmp(p,"exit"))
			event_base_loopexit(bufferevent_get_base(bev),NULL);
		else if (!strcmp(p,"quit"))
			event_base_loopbreak(bufferevent_get_base(bev));

		printf("recv data:%s\n", p);  

		int n = sprintf_s(msg,"srv recv data:%s\n",p);
		//发送消息给客户端
		bufferevent_write(bev, msg, n );

		// 这里记得把分配的内存释放掉，不然会内存泄漏
		free(p);
	}
}

void socket_event_cb(bufferevent *bev, short events, void *arg)  
{  
	if (events & BEV_EVENT_EOF)  
		printf("connection closed\n");  
	else if (events & BEV_EVENT_ERROR)  
		printf("some other error\n");  

	//这将自动close套接字和free读写缓冲区  
	bufferevent_free(bev);  
}