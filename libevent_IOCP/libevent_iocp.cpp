//��������Ҫ��ͷ�ļ�
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/thread.h"
#include "event2/buffer.h"

//�����ص�����
void listener_cb(evconnlistener *listener, evutil_socket_t fd,
struct sockaddr *sock, int socklen, void *arg);  

//��Socket������Ϣ�Ļص�����
void socket_read_cb(bufferevent *bev, void *arg);

//��Socket�¼��Ļص�����
void socket_event_cb(bufferevent *bev, short events, void *arg);  

int main()
{  
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);
	//��������ʼ�����磬��Ȼ�ᴴ��Socketʧ��
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

	//����libEventʹ��Windows�߳�
	//����Ǳ���ģ���Ȼ�ᵼ��event_base_dispatchʱһֱ����Sleep״̬���޷���������
	evthread_use_windows_threads();

	struct event_config* cfg = event_config_new();
	event_config_set_flag(cfg,EVENT_BASE_FLAG_STARTUP_IOCP);
	//����CPUʵ����������libEvent��CPU��
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	event_config_set_num_cpus_hint(cfg,si.dwNumberOfProcessors);

	event_base *base;
	base = event_base_new_with_config(cfg); 
	event_config_free(cfg);

	// �󶨲�����IPV4�˿�
	evconnlistener *listener = evconnlistener_new_bind(base, listener_cb, base,  
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,  
		10, (struct sockaddr*)&sin,  
		sizeof(sin));

	/*
	// �󶨲�����IPV6�˿�
	evconnlistener *listener6 = evconnlistener_new_bind(base, listener_cb, base,  
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,  
		10, (struct sockaddr*)&sin6,  
		sizeof(sin6));
	*/

	//�¼��ַ�����
	event_base_dispatch(base);  

	evconnlistener_free(listener);
	//evconnlistener_free(listener6);
	event_base_free(base);
	WSACleanup();

	return 0;  
}

//һ���¿ͻ��������Ϸ�������  
//���˺���������ʱ��libevent�Ѿ�������accept������ͻ��ˡ��ÿͻ��˵�
//�ļ�������Ϊfd
void listener_cb(evconnlistener *listener, evutil_socket_t fd,  
struct sockaddr *sock, int socklen, void *arg)  
{  
	char Buffer[256];
	sockaddr_in* addr = (sockaddr_in*)sock;
	evutil_inet_ntop(addr->sin_family,&addr->sin_addr,Buffer,sizeof(Buffer));
	printf("accept a client %d,IP:%s\n", fd,Buffer);

	event_base *base = (event_base*)arg;  

	//Ϊ����ͻ��˷���һ��bufferevent  
	bufferevent *bev =  bufferevent_socket_new(base, fd,  
		BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);  

	bufferevent_setcb(bev, socket_read_cb, NULL, socket_event_cb, NULL);  
	bufferevent_enable(bev, EV_READ | EV_PERSIST);  
}

void socket_read_cb(bufferevent *bev, void *arg)  
{  
	char msg[4096];

	size_t len;
	// ����һ��һ�еĶ�ȡ
	char* p = evbuffer_readln(bufferevent_get_input(bev),&len,EVBUFFER_EOL_ANY);
	if(p)
	{
		// �������exit����quit���˳�����
		// ����ʹ��event_base_loopexit����event_base_loopbreak
		// ���ǵ�������ǰ�߻���¼���������˳��������������˳�
		if(!strcmp(p,"exit"))
			event_base_loopexit(bufferevent_get_base(bev),NULL);
		else if (!strcmp(p,"quit"))
			event_base_loopbreak(bufferevent_get_base(bev));

		printf("recv data:%s\n", p);  

		int n = sprintf_s(msg,"srv recv data:%s\n",p);
		//������Ϣ���ͻ���
		bufferevent_write(bev, msg, n );

		// ����ǵðѷ�����ڴ��ͷŵ�����Ȼ���ڴ�й©
		free(p);
	}
}

void socket_event_cb(bufferevent *bev, short events, void *arg)  
{  
	if (events & BEV_EVENT_EOF)  
		printf("connection closed\n");  
	else if (events & BEV_EVENT_ERROR)  
		printf("some other error\n");  

	//�⽫�Զ�close�׽��ֺ�free��д������  
	bufferevent_free(bev);  
}